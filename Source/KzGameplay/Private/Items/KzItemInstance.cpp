// Copyright 2026 kirzo

#include "Items/KzItemInstance.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemFragment.h"
#include "Inventory/KzInventoryComponent.h"

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

void FKzItemInstance::PostReplicatedAdd(const FKzInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnInventoryChanged.Broadcast();
	}
}

void FKzItemInstance::PostReplicatedChange(const FKzInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnInventoryChanged.Broadcast();
	}
}

void FKzItemInstance::PreReplicatedRemove(const FKzInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->OnInventoryChanged.Broadcast();
	}
}