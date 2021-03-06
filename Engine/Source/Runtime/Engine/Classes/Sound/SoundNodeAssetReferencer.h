// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Sound/SoundNode.h"
#include "SoundNodeAssetReferencer.generated.h"

/** 
 * Sound node that contains a reference to the raw wave file to be played
 */
UCLASS(abstract)
class ENGINE_API USoundNodeAssetReferencer : public USoundNode
{
	GENERATED_BODY()

public:
	virtual void LoadAsset() PURE_VIRTUAL(USoundNodesAssetReferencer::LoadAsset,);

#if WITH_EDITOR
	virtual void PostEditImport() override;
#endif
};

