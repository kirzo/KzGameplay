// Copyright 2026 kirzo

#include "Inventory/InventoryComponent.h"
#include "Items/ItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Capacity = 20; // Default slots
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly to save bandwidth,
	DOREPLIFETIME_CONDITION(UInventoryComponent, Items, COND_OwnerOnly);
}

void UInventoryComponent::OnRep_Items()
{
	// Notify the UI or other systems on the client that the inventory has updated
	OnInventoryChanged.Broadcast();
}

bool UInventoryComponent::HasSpace() const
{
	return Items.Num() < Capacity;
}

int32 UInventoryComponent::FindStackableSlot(const UItemDefinition* ItemDef) const
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

bool UInventoryComponent::TryAddItem(const UItemDefinition* ItemDef, int32 Quantity, AActor* PhysicalActor)
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
		Items.Add(FItemInstance(ItemDef, AmountToAdd, nullptr));

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

		// 4. Trigger Scriptable Framework Actions (e.g., granting a passive buff)
		//FScriptableAction::RunAction(this, ItemDef->OnAcquiredAction);

		// 5. Notify server-side listeners
		OnInventoryChanged.Broadcast();

		return true;
	}

	return false;
}

bool UInventoryComponent::RemoveItem(const UItemDefinition* ItemDef, int32 Quantity)
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
		// Trigger Scriptable Framework Actions (e.g., removing a passive buff)
		// ItemDef->OnRemovedAction.ExecuteAction(GetOwner());

		OnInventoryChanged.Broadcast();
		return true;
	}

	return false;
}