// Copyright 2026 kirzo

#include "Factories/EquipmentLayoutAssetFactory.h"
#include "Equipment/KzEquipmentLayout.h"

UEquipmentLayoutAssetFactory::UEquipmentLayoutAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzEquipmentLayout::StaticClass();
}

UObject* UEquipmentLayoutAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzEquipmentLayout>(InParent, Class, Name, Flags | RF_Transactional);
}