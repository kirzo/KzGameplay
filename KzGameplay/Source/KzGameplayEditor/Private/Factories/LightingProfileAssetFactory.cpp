// Copyright 2026 kirzo

#include "Factories/LightingProfileAssetFactory.h"
#include "KzLightingProfile.h"

ULightingProfileAssetFactory::ULightingProfileAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzLightingProfile::StaticClass();
}

UObject* ULightingProfileAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzLightingProfile>(InParent, Class, Name, Flags | RF_Transactional);
}