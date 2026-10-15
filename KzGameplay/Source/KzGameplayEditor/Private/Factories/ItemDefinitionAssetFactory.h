// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ItemDefinitionAssetFactory.generated.h"

UCLASS()
class UItemDefinitionAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UItemDefinitionAssetFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};