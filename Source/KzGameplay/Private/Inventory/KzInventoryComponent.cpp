// Copyright 2026 kirzo

#include "Inventory/KzInventoryComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/Fragments/KzItemFragment_Storable.h"
#include "Items/KzItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

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

	const UKzItemFragment_Storable* StoreFrag = ItemDef->FindFragmentByClass<UKzItemFragment_Storable>();
	if (!StoreFrag) return INDEX_NONE;

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].ItemDef == ItemDef && Items[i].Quantity < StoreFrag->MaxStackSize)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool UKzInventoryComponent::TryAddItem(const UKzItemDefinition* ItemDef, int32 Quantity, AActor* PhysicalActor)
{
	if (!ItemDef || Quantity <= 0)
	{
		return false;
	}

	// Create a fresh instance and route it through the master function.
	// We pass nullptr for SpawnedActor internally because the item is going into the backpack,
	// but we still pass PhysicalActor to the function to handle the world cleanup.
	FKzItemInstance FreshInstance(ItemDef, Quantity, nullptr);

	return TryAddInstance(FreshInstance, PhysicalActor);
}

bool UKzInventoryComponent::TryAddInstance(const FKzItemInstance& Instance, AActor* PhysicalActor)
{
	if (!GetOwner()->HasAuthority() || !Instance.IsValid())
	{
		return false;
	}

	const UKzItemFragment_Storable* StoreFrag = Instance.ItemDef->FindFragmentByClass<UKzItemFragment_Storable>();
	if (!StoreFrag)
	{
		return false;
	}

	int32 RemainingQuantity = Instance.Quantity;

	// 1. Try to fill existing stacks first
	// Note: If items are stackable, their unique stats (if any) will be absorbed by the existing stack.
	// It is highly recommended that items with dynamic stats have a MaxStackSize of 1.
	while (RemainingQuantity > 0)
	{
		int32 StackIndex = FindStackableSlot(Instance.ItemDef);
		if (StackIndex == INDEX_NONE) break;

		int32 AvailableSpaceInStack = StoreFrag->MaxStackSize - Items[StackIndex].Quantity;
		int32 AmountToAdd = FMath::Min(RemainingQuantity, AvailableSpaceInStack);

		Items[StackIndex].Quantity += AmountToAdd;
		RemainingQuantity -= AmountToAdd;
	}

	// 2. Create new slots for the remaining quantity
	while (RemainingQuantity > 0 && HasSpace())
	{
		int32 AmountToAdd = FMath::Min(RemainingQuantity, StoreFrag->MaxStackSize);

		// Add a copy of the live instance to preserve dynamic stats
		FKzItemInstance& NewInstance = Items.Add_GetRef(Instance);
		NewInstance.Quantity = AmountToAdd;
		NewInstance.SpawnedActor = nullptr; // Erase physical references since it's now stored
		NewInstance.SpawnedComponent = nullptr;

		// Initialize Acquired Actions
		NewInstance.ActiveAcquiredAction = StoreFrag->OnAcquiredAction.Clone(this);
		NewInstance.ActiveAcquiredAction.SetContextProperty(TEXT("Instigator"), GetOwner());
		NewInstance.ActiveAcquiredAction.SetContextProperty(TEXT("Inventory"), this);

		NewInstance.ActiveAcquiredAction.Run(this);

		RemainingQuantity -= AmountToAdd;
	}

	// 3. Process results
	if (RemainingQuantity < Instance.Quantity)
	{
		int32 TotalAdded = Instance.Quantity - RemainingQuantity;

		// Grant passive tags
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
		{
			for (const FGameplayTag& Tag : StoreFrag->InventoryTags)
			{
				ASC->AddLooseGameplayTag(Tag, TotalAdded);
			}
		}

		// Handle the physical actor (cleanup or update remaining quantity)
		if (PhysicalActor)
		{
			if (RemainingQuantity == 0)
			{
				PhysicalActor->Destroy();
			}
			else
			{
				// If we couldn't fit everything, update the remaining quantity on the physical item
				if (UKzItemComponent* ItemComp = PhysicalActor->FindComponentByClass<UKzItemComponent>())
				{
					ItemComp->ItemInstance.Quantity = RemainingQuantity;
				}
			}
		}

		OnInventoryChanged.Broadcast();
		return true;
	}

	return false;
}

bool UKzInventoryComponent::RemoveItem(const UKzItemDefinition* ItemDef, int32 Quantity)
{
	const UKzItemFragment_Storable* StoreFrag = ItemDef ? ItemDef->FindFragmentByClass<UKzItemFragment_Storable>() : nullptr;
	if (!StoreFrag || Quantity <= 0 || !GetOwner()->HasAuthority()) return false;

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
		// Calculate exactly how many items were actually removed from the backpack
		int32 TotalRemoved = Quantity - RemainingToRemove;

		// Remove the Inventory Tags from the owner's Ability System Component
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
		{
			for (const FGameplayTag& Tag : StoreFrag->InventoryTags)
			{
				// Remove the exact count we discarded so the reference count drops correctly.
				ASC->RemoveLooseGameplayTag(Tag, TotalRemoved);
			}
		}

		OnInventoryChanged.Broadcast();
		return true;
	}

	return false;
}