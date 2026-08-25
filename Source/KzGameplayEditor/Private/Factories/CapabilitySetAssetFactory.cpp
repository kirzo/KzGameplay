// Copyright 2026 kirzo

#include "Factories/CapabilitySetAssetFactory.h"
#include "Capabilities/KzCapabilitySet.h"

UCapabilitySetAssetFactory::UCapabilitySetAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzCapabilitySet::StaticClass();
}

UObject* UCapabilitySetAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzCapabilitySet>(InParent, Class, Name, Flags | RF_Transactional);
}
