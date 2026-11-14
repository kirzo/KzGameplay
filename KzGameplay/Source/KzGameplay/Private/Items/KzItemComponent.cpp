// Copyright 2026 kirzo

#include "Items/KzItemComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemFragment.h"
#include "Items/Fragments/KzItemFragment_Storable.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Inventory/KzInventoryComponent.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Interaction/KzInteractorComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/AssetManager.h"

AActor* FKzItemHolder::GetActor() const
{
	return Equipment ? Equipment->GetOwner() : nullptr;
}

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

	// On every machine, since what the fragments load, such as a held pose, plays on all of them.
	if (const UKzItemDefinition* Item = ItemInstance.ItemDef)
	{
		TArray<FSoftObjectPath> Paths;
		for (const UKzItemFragment* Fragment : Item->Fragments)
		{
			if (Fragment)
			{
				Fragment->GetPreloadPaths(Paths);
			}
		}
		if (!Paths.IsEmpty())
		{
			PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Paths));
		}
	}
}

void UKzItemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Destroyed while held, as a rope that burns: the slots holding it let go of it, which replicates to everyone
	if (EndPlayReason == EEndPlayReason::Destroyed && GetOwner()->HasAuthority())
	{
		for (const FKzItemHolder& Holder : TArray<FKzItemHolder>(Holders))
		{
			if (Holder.Equipment)
			{
				FKzItemInstance Unequipped;
				Holder.Equipment->UnequipItem(Holder.SlotID, Unequipped);
			}
		}
	}

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

bool UKzItemComponent::IsHeldBy(const AActor* Actor) const
{
	return Actor && Holders.ContainsByPredicate([Actor](const FKzItemHolder& Holder) { return Holder.GetActor() == Actor; });
}

bool UKzItemComponent::HasRoomForHolder() const
{
	const UKzItemFragment_Equippable* Equippable = ItemInstance.ItemDef ? ItemInstance.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
	return Holders.Num() < (Equippable ? Equippable->MaxHolders : 1);
}

void UKzItemComponent::AddHolder(const FKzItemHolder& Holder, const FKzTransformSource& AttachSource)
{
	Holders.Add(Holder);
	OnHeld.Broadcast(Holder, AttachSource);
}

void UKzItemComponent::RemoveHolder(const UKzEquipmentComponent* Equipment, FGameplayTag SlotID)
{
	const int32 Index = Holders.IndexOfByPredicate([Equipment, SlotID](const FKzItemHolder& Holder) { return Holder.Equipment.Get() == Equipment && Holder.SlotID == SlotID; });
	if (Index != INDEX_NONE)
	{
		const FKzItemHolder Holder = Holders[Index];
		Holders.RemoveAt(Index);
		OnReleased.Broadcast(Holder);
	}
}

FVector UKzItemComponent::GetItemVelocity() const
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;
	for (const FKzItemHolder& Holder : Holders)
	{
		if (const AActor* Actor = Holder.GetActor())
		{
			Sum += Actor->GetVelocity();
			++Count;
		}
	}

	if (Count > 0)
	{
		return Sum / Count;
	}

	if (AActor* OwnerActor = GetOwner())
	{
		return OwnerActor->GetVelocity();
	}

	return FVector::ZeroVector;
}

bool UKzItemComponent::CanInteract_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	if (!HasRoomForHolder())
	{
		return false;
	}

	// The interactor may take it into another slot of its own, while it has one left that does not hold it already
	const AActor* InteractorActor = Interactor ? Interactor->GetOwner() : nullptr;
	if (!IsHeldBy(InteractorActor))
	{
		return true;
	}

	const UKzEquipmentComponent* Equipment = InteractorActor->FindComponentByClass<UKzEquipmentComponent>();
	return Equipment && Equipment->FindSlotFor(ItemInstance.ItemDef, GetOwner()).IsValid();
}

EKzInteractionResult UKzItemComponent::GetInteractionResult_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	// Picking up is instant. Whether it fits is not predicted here: that is what the interactable's
	// availability rules are for, and they can say so on the prompt instead of failing silently
	const bool bCanPickUp = Interactor && ItemInstance.IsValid() && GetOwner() && GetOwner()->HasAuthority();
	return bCanPickUp ? EKzInteractionResult::Completed : EKzInteractionResult::Ignored;
}

TMap<FString, FText> UKzItemComponent::GetPromptArguments_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	TMap<FString, FText> Arguments;
	if (ItemInstance.ItemDef)
	{
		Arguments.Add(TEXT("Item"), ItemInstance.ItemDef->DisplayName);
	}
	return Arguments;
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
			bool bEquipped = EquipmentComp->EquipItemFromWorld(this, UnequippedItem, Interactable);

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