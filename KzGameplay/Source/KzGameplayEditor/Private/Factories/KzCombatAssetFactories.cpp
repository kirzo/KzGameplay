// Copyright 2026 kirzo

#include "Factories/KzCombatAssetFactories.h"
#include "Combat/KzAttackSet.h"
#include "Combat/KzCombatConfig.h"

UKzCombatConfigFactory::UKzCombatConfigFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzCombatConfig::StaticClass();
}

UObject* UKzCombatConfigFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzCombatConfig>(InParent, Class, Name, Flags | RF_Transactional);
}

UKzAttackSetFactory::UKzAttackSetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzAttackSet::StaticClass();
}

UObject* UKzAttackSetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzAttackSet>(InParent, Class, Name, Flags | RF_Transactional);
}