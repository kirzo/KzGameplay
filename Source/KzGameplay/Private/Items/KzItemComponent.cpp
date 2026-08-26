// Copyright 2026 kirzo

#include "Items/KzItemComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/Fragments/KzItemFragment_Storable.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Inventory/KzInventoryComponent.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Interaction/KzInteractorComponent.h"
#include "GameFramework/Actor.h"

UKzItemComponent::UKzItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UKzItemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* OwnerActor = GetOwner())
	{
		if (!bSimulatePhysics)
		{
			if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
			{
				bSimulatePhysics = RootPrim->IsSimulatingPhysics();
			}
		}

		if (OwnerActor->HasAuthority())
		{
			// Link the physical world actor to the instance
			ItemInstance.SpawnedActor = OwnerActor;

			// If this component was placed in the world via the editor, its ItemInstance might have an ItemDef
			// but no initialized stats. We ensure it's properly initialized here.
			if (ItemInstance.ItemDef && ItemInstance.Quantity > 0 && ItemInstance.Stats.Items.IsEmpty())
			{
				ItemInstance.Initialize(ItemInstance.ItemDef);
			}
		}
	}
}

bool UKzItemComponent::CanInteract_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	return !EquipperActor.IsValid();
}

EKzInteractionResult UKzItemComponent::GetInteractionResult_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	// Picking up is instant. Whether it fits is not predicted here: that is what the interactable's
	// availability rules are for, and they can say so on the prompt instead of failing silently
	const bool bCanPickUp = Interactor && ItemInstance.IsValid() && GetOwner() && GetOwner()->HasAuthority();
	return bCanPickUp ? EKzInteractionResult::Completed : EKzInteractionResult::Ignored;
}

void UKzItemComponent::OnInteractionBegun_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction)
{
	if (!GetOwner()->HasAuthority() || !ItemInstance.IsValid() || !Interactor)
	{
		return;
	}

	bool bHandled = false;
	AActor* InteractorActor = Interactor->GetOwner();

	const UKzItemFragment_Equippable* EquipFrag = ItemInstance.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
	const UKzItemFragment_Storable* StoreFrag = ItemInstance.ItemDef->FindFragmentByClass<UKzItemFragment_Storable>();

	// 1. Try to Auto-Equip first (if the item allows it)
	if (EquipFrag && (EquipFrag->bAutoEquip || !StoreFrag))
	{
		if (UKzEquipmentComponent* EquipmentComp = InteractorActor->FindComponentByClass<UKzEquipmentComponent>())
		{
			FKzItemInstance UnequippedItem;
			bool bEquipped = EquipmentComp->EquipItemFromWorld(this, UnequippedItem);

			if (bEquipped)
			{
				OnPickedUp.Broadcast(InteractorActor);
				bHandled = true;
			}
		}
	}

	// 2. If it wasn't equipped (not auto-equip, or equipment full/failed), send to Backpack
	if (!bHandled && StoreFrag)
	{
		if (UKzInventoryComponent* InvComp = InteractorActor->FindComponentByClass<UKzInventoryComponent>())
		{
			// TryAddItem will automatically destroy GetOwner() if it fits in the backpack
			InvComp->TryAddInstance(ItemInstance, GetOwner());
		}
	}
}

void UKzItemComponent::SetEquippedState(AActor* NewEquipper, FGameplayTag NewSlotID)
{
	EquipperActor = NewEquipper;
	EquippedSlotID = NewSlotID;
}

void UKzItemComponent::ClearEquippedState()
{
	EquipperActor = nullptr;
	EquippedSlotID = FGameplayTag::EmptyTag;
}

FVector UKzItemComponent::GetItemVelocity() const
{
	if (AActor* Equipper = EquipperActor.Get())
	{
		return Equipper->GetVelocity();
	}

	if (AActor* OwnerActor = GetOwner())
	{
		return OwnerActor->GetVelocity();
	}

	return FVector::ZeroVector;
}