// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "GenesisMetaImporterPrivatePCH.h"
#include "GenesisMetaImportFactory.h"

#include "UnrealEd.h"

#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "MainFrame.h"
#include "ModuleManager.h"
#include "ObjectTools.h"
#include "PackageTools.h"

#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "GenesisMetaImportFactory"

DEFINE_LOG_CATEGORY_STATIC(GenesisMetaImport, Log, All);

UGenesisMetaImportFactory::UGenesisMetaImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UWorld::StaticClass();

	bEditorImport = true;
	bText = true;

	Formats.Add(TEXT("gns;GenesisMeta"));
}

FText UGenesisMetaImportFactory::GetDisplayName() const
{
	return LOCTEXT("GenesisMetaImportFactoryDescription", "GenesisMeta");
}

bool UGenesisMetaImportFactory::DoesSupportClass(UClass * Class)
{
	return (Class == UWorld::StaticClass());
}

UClass* UGenesisMetaImportFactory::ResolveSupportedClass()
{
	return UWorld::StaticClass();
}

UObject* UGenesisMetaImportFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, Type);

	FString JSONString;
	if (FFileHelper::LoadFileToString(JSONString, *UFactory::CurrentFilename))
	{
		TSharedRef<TJsonReader<TCHAR>> JSONReader = TJsonReaderFactory<TCHAR>::Create(JSONString);

		if (FJsonSerializer::Deserialize(JSONReader, JSONDataArray))
		{
			// Editor modes cannot be active when any level saving occurs
			GLevelEditorModeTools().DeactivateAllModes();
		
			// Get level and package name from the imported file name
			FString LevelName = ObjectTools::SanitizeObjectName(InName.ToString());
			FString NewPackageName = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetName()) + TEXT("/") + LevelName;
			NewPackageName = PackageTools::SanitizePackageName(NewPackageName);

			// Create a new world
			UWorldFactory* Factory = NewObject<UWorldFactory>();
			Factory->WorldType = EWorldType::Inactive;
			UPackage* Package = CreatePackage(NULL, *NewPackageName);
			FName WorldName = FName(*LevelName);
			EObjectFlags WorldFlags = RF_Public | RF_Standalone;
			UWorld* NewWorld = CastChecked<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Package, WorldName, WorldFlags, NULL, GWarn));
			if (NewWorld)
			{
				FAssetRegistryModule::AssetCreated(NewWorld);
			}

			for (int i = 0; i < JSONDataArray.Num(); i++)
			{

			}

			// Save world to disk
			const bool bNewWorldSaved = FEditorFileUtils::SaveLevel(NewWorld->PersistentLevel, NewPackageName);
			if (bNewWorldSaved) 
			{
				FEditorDelegates::OnAssetPostImport.Broadcast(this, NewWorld);
				return NewWorld;
			}
		}
		else
		{
			UE_LOG(GenesisMetaImport, Error, TEXT("JSON Error: %d"), JSONReader->GetLineNumber());
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE