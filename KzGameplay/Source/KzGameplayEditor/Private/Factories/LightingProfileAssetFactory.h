// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LightingProfileAssetFactory.generated.h"

UCLASS()
class ULightingProfileAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	ULightingProfileAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};