// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class GenesisMetaImporter : ModuleRules
    {
        public GenesisMetaImporter(TargetInfo Target)
        {
            PublicIncludePaths.AddRange(
                new string[] {
					// ... add public include paths required here ...
				}
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "MainFrame",
                    "Editor/GenesisMetaImporter/Private",
					// ... add other private include paths required here ...
				}
                );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "Json",
                    "Slate",
                    "SlateCore",
                    "EditorStyle",
                    "InputCore",
                    "RawMesh",
                    "UnrealEd",
                    "MainFrame"
					// ... add other public dependencies that you statically link with here ...
				}
                );

            PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "AssetTools",
                "AssetRegistry"
            });

            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                "AssetTools",
                "AssetRegistry"
            });
        }
    }
}