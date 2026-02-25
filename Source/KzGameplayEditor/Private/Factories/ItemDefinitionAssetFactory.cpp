// Copyright 2026 kirzo

#include "Factories/ItemDefinitionAssetFactory.h"
#include "Items/ItemDefinition.h"

UItemDefinitionAssetFactory::UItemDefinitionAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UItemDefinition::StaticClass();
}

UObject* UItemDefinitionAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UItemDefinition>(InParent, Class, Name, Flags | RF_Transactional);
}