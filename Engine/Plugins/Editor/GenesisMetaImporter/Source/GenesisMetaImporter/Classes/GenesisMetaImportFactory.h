// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
* Factory for importing Genesis Noir model metadata files
*/

#include "GenesisMetaImportFactory.generated.h"

UCLASS(hidecategories = Object)
class UGenesisMetaImportFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UFactory Interface
	virtual FText GetDisplayName() const override;
	virtual bool DoesSupportClass(UClass * Class) override;
	virtual UClass* ResolveSupportedClass() override;
	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) override;

	//TSet<UPackage*> LoadedPackages;

	TArray<TSharedPtr<FJsonValue>> JSONDataArray;
};

#pragma once
