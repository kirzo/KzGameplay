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
	Quantity = 1;
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
	}
}

EKzInteractionResult UKzItemComponent::HandleInteraction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	if (!GetOwner()->HasAuthority() || !ItemDef || Quantity <= 0 || !Interactor)
	{
		return EKzInteractionResult::Ignored;
	}

	EKzInteractionResult Result = EKzInteractionResult::Ignored;
	AActor* InteractorActor = Interactor->GetOwner();

	const UKzItemFragment_Equippable* EquipFrag = ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
	const UKzItemFragment_Storable* StoreFrag = ItemDef->FindFragmentByClass<UKzItemFragment_Storable>();

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
				Result = EKzInteractionResult::Completed;
			}
		}
	}

	// 2. If it wasn't equipped (not auto-equip, or equipment full/failed), send to Backpack
	if (Result == EKzInteractionResult::Ignored && StoreFrag)
	{
		if (UKzInventoryComponent* InvComp = InteractorActor->FindComponentByClass<UKzInventoryComponent>())
		{
			// TryAddItem will automatically destroy GetOwner() if it fits in the backpack
			bool bItemAdded = InvComp->TryAddItem(ItemDef, Quantity, GetOwner());
			if (bItemAdded)
			{
				Result = EKzInteractionResult::Completed;
			}
		}
	}

	return Result;
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