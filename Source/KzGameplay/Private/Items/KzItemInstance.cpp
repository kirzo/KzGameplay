// Copyright 2026 kirzo

#include "Items/KzItemInstance.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemFragment.h"

FKzItemInstance::FKzItemInstance(const UKzItemDefinition* InDef, int32 InQuantity, AActor* InSpawnedActor)
	: Quantity(InQuantity)
	, SpawnedActor(InSpawnedActor)
{
	Initialize(InDef);
}

void FKzItemInstance::Initialize(const UKzItemDefinition* ItemDefinition)
{
	ItemDef = ItemDefinition;
	if (ItemDef)
	{
		for (const UKzItemFragment* Fragment : ItemDef->Fragments)
		{
			if (Fragment)
			{
				Fragment->OnInstanceCreated(*this);
			}
		}
	}
}