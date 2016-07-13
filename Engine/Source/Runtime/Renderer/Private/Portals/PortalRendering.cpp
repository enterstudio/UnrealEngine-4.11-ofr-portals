// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CustomDepthRendering.cpp: CustomDepth rendering implementation.
 =============================================================================*/

#include "RendererPrivate.h"
#include "Engine.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "PostProcessing.h"
#include "RenderingCompositionGraph.h"
#include "SceneFilterRendering.h"
#include "SceneUtils.h"

/*-----------------------------------------------------------------------------
	FPortalPrimSet
 -----------------------------------------------------------------------------*/

bool FPortalPrimSet::DrawPrims(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
    bool bDirty=false;
    
    // If we don't have any prims to render, then uh, don't render any prims, ya dummy
    if(Prims.Num())
    {   
        // Clear the stencil buffer
        RHICmdList.Clear(false, FLinearColor(), false, 0.0f, true, 0, FIntRect());

		// Set the stencil buffer state, write 1's
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
			/* Enable Depth Write = */ true,
			/* Depth Test Method = */ CF_DepthNearOrEqual,
			/* Enable Front Face Stecil = */ true,
			/* Front Face Stencil Test Method = */ CF_Always,
			/* Front Face Stencil Test Fail Op = */ SO_Keep,
			/* Front Face Depth Test Fail Op = */ SO_Keep,
			/* Front Face Stencil Test Pass Op = */ SO_Replace,
			/* Enable Back Face Stencil = */  false,
			/* Back Face Stencil Test Method = */ CF_Always,
			/* Back Face Stencil Test Fail Op = */ SO_Keep,
			/* Back Face Depth Test Fail Op = */ SO_Keep,
			/* Back Face Stencil Test Pass Op = */ SO_Keep,
			/* Stencil Read Mask = */ 0xFF,
			/* Stencil Write Mask = */ 0xFF>::GetRHI(), 128);
        
        // Loop through all the prims and render them
        for (int32 PrimIdx = 0; PrimIdx < Prims.Num(); PrimIdx++)
        {
            FPrimitiveSceneProxy* PrimitiveSceneProxy = Prims[PrimIdx];
            const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy->GetPrimitiveSceneInfo();
            
            // If the prim is visible, maybe try rendering it
            if (View.PrimitiveVisibilityMap[PrimitiveSceneInfo->GetIndex()])
            {
                const FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveSceneInfo->GetIndex()];
                
                FDepthDrawingPolicyFactory::ContextType Context(DDM_AllOpaque);

                UE_LOG(LogTemp, Warning, TEXT("Gonna try rendering this thing to the stencil buffer...?"));

                // Draw dynamic meshes
                for (int32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.Num(); MeshBatchIndex++)
                {
                    const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];
                    
                    if (MeshBatchAndRelevance.PrimitiveSceneProxy == PrimitiveSceneProxy)
                    {
                        const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;
                        FDepthDrawingPolicyFactory::DrawDynamicMesh(
							RHICmdList, 
							View, 
							Context, 
							MeshBatch,
							false, 
							true,
							MeshBatchAndRelevance.PrimitiveSceneProxy, 
							MeshBatch.BatchHitProxyId
						);
                    }
                }
                
                // Draw static meshes
                if (ViewRelevance.bStaticRelevance)
                {
                    for (int32 StaticMeshIdx = 0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++)
                    {
                        const FStaticMesh& StaticMesh = PrimitiveSceneInfo->StaticMeshes[StaticMeshIdx];
                        
                        if (View.StaticMeshVisibilityMap[StaticMesh.Id])
                        {
                            const FMeshDrawingRenderState DrawRenderState(View.GetDitheredLODTransitionState(StaticMesh));
                            bDirty |= FDepthDrawingPolicyFactory::DrawStaticMesh(
								RHICmdList, 
								View,
								Context,
								StaticMesh,
								StaticMesh.Elements.Num() == 1 ? 1 : View.StaticMeshBatchVisibility[StaticMesh.Id],
								true,
								DrawRenderState,
								PrimitiveSceneProxy,
								StaticMesh.BatchHitProxyId
							);
                        }
                    }
                }
            }
        }
    }
    
    return bDirty;
}
