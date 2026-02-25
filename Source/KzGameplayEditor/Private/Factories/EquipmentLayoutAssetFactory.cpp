// Copyright 2026 kirzo

#include "Factories/EquipmentLayoutAssetFactory.h"
#include "Equipment/EquipmentLayout.h"

UEquipmentLayoutAssetFactory::UEquipmentLayoutAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UEquipmentLayout::StaticClass();
}

UObject* UEquipmentLayoutAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UEquipmentLayout>(InParent, Class, Name, Flags | RF_Transactional);
}