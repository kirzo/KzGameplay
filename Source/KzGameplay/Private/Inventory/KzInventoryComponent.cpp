// Copyright 2026 kirzo

#include "Inventory/KzInventoryComponent.h"
#include "Items/KzItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UKzInventoryComponent::UKzInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Capacity = 20; // Default slots
}

void UKzInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly to save bandwidth,
	DOREPLIFETIME_CONDITION(UKzInventoryComponent, Items, COND_OwnerOnly);
}

void UKzInventoryComponent::OnRep_Items()
{
	// Notify the UI or other systems on the client that the inventory has updated
	OnInventoryChanged.Broadcast();
}

bool UKzInventoryComponent::HasSpace() const
{
	return Items.Num() < Capacity;
}

int32 UKzInventoryComponent::FindStackableSlot(const UKzItemDefinition* ItemDef) const
{
	if (!ItemDef) return INDEX_NONE;

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].ItemDef == ItemDef && Items[i].Quantity < ItemDef->MaxStackSize)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool UKzInventoryComponent::TryAddItem(const UKzItemDefinition* ItemDef, int32 Quantity, AActor* PhysicalActor)
{
	if (!ItemDef || Quantity <= 0 || !GetOwner()->HasAuthority())
	{
		return false;
	}

	int32 RemainingQuantity = Quantity;

	// 1. Try to fill existing stacks first
	while (RemainingQuantity > 0)
	{
		int32 StackIndex = FindStackableSlot(ItemDef);
		if (StackIndex == INDEX_NONE)
		{
			break; // No more stackable slots available
		}

		int32 AvailableSpaceInStack = ItemDef->MaxStackSize - Items[StackIndex].Quantity;
		int32 AmountToAdd = FMath::Min(RemainingQuantity, AvailableSpaceInStack);

		Items[StackIndex].Quantity += AmountToAdd;
		RemainingQuantity -= AmountToAdd;
	}

	// 2. Create new slots for the remaining quantity
	while (RemainingQuantity > 0 && HasSpace())
	{
		int32 AmountToAdd = FMath::Min(RemainingQuantity, ItemDef->MaxStackSize);

		// Note: We deliberately pass nullptr for the PhysicalActor because it's going into the backpack
		FKzItemInstance& NewInstance = Items.Add_GetRef(FKzItemInstance(ItemDef, AmountToAdd, nullptr));

		NewInstance.ActiveAcquiredAction = ItemDef->OnAcquiredAction.Clone(this);
		NewInstance.ActiveAcquiredAction.SetContextProperty(TEXT("Instigator"), GetOwner());
		NewInstance.ActiveAcquiredAction.SetContextProperty(TEXT("Inventory"), this);

		NewInstance.ActiveAcquiredAction.Run(this);

		RemainingQuantity -= AmountToAdd;
	}

	// If we successfully added at least *some* items
	if (RemainingQuantity < Quantity)
	{
		// 3. World Cleanup Rule: Destroy the physical actor if it went into the backpack
		if (IsValid(PhysicalActor))
		{
			PhysicalActor->Destroy();
		}

		// 4. Notify server-side listeners
		OnInventoryChanged.Broadcast();

		return true;
	}

	return false;
}

bool UKzInventoryComponent::RemoveItem(const UKzItemDefinition* ItemDef, int32 Quantity)
{
	if (!ItemDef || Quantity <= 0 || !GetOwner()->HasAuthority())
	{
		return false;
	}

	int32 RemainingToRemove = Quantity;

	// Iterate backwards so we can safely remove empty slots
	for (int32 i = Items.Num() - 1; i >= 0; --i)
	{
		if (Items[i].ItemDef == ItemDef)
		{
			int32 AmountToRemove = FMath::Min(RemainingToRemove, Items[i].Quantity);
			Items[i].Quantity -= AmountToRemove;
			RemainingToRemove -= AmountToRemove;

			if (Items[i].Quantity <= 0)
			{
				Items[i].ActiveAcquiredAction.Reset();
				Items.RemoveAt(i);
			}

			if (RemainingToRemove <= 0)
			{
				break;
			}
		}
	}

	if (RemainingToRemove < Quantity) // We removed at least something
	{
		OnInventoryChanged.Broadcast();
		return true;
	}

	return false;
}