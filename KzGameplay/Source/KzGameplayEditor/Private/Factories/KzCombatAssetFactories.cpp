// Copyright 2026 kirzo

#include "Factories/KzCombatAssetFactories.h"
#include "Combat/KzAttackSet.h"
#include "Combat/KzCombatConfig.h"
#include "Combat/KzTimeStop.h"
#include "Engagement/KzEngagementConfig.h"

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

UKzEngagementConfigFactory::UKzEngagementConfigFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzEngagementConfig::StaticClass();
}

UObject* UKzEngagementConfigFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzEngagementConfig>(InParent, Class, Name, Flags | RF_Transactional);
}

UKzHitStopFactory::UKzHitStopFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzHitStop::StaticClass();
}

UObject* UKzHitStopFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzHitStop>(InParent, Class, Name, Flags | RF_Transactional);
}

UKzSlowMotionFactory::UKzSlowMotionFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzSlowMotion::StaticClass();
}

UObject* UKzSlowMotionFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzSlowMotion>(InParent, Class, Name, Flags | RF_Transactional);
}