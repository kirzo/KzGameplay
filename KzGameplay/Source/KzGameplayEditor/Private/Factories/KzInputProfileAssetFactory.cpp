// Copyright 2026 kirzo

#include "Factories/KzInputProfileAssetFactory.h"
#include "Input/KzInputProfile.h"

UKzInputProfileAssetFactory::UKzInputProfileAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzInputProfile::StaticClass();
}

UObject* UKzInputProfileAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzInputProfile>(InParent, Class, Name, Flags | RF_Transactional);
}