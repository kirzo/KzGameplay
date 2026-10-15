// Copyright 2026 kirzo

#include "Factories/ItemDefinitionAssetFactory.h"
#include "Items/KzItemDefinition.h"

UItemDefinitionAssetFactory::UItemDefinitionAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzItemDefinition::StaticClass();
}

UObject* UItemDefinitionAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzItemDefinition>(InParent, Class, Name, Flags | RF_Transactional);
}