// Copyright 2026 kirzo

#include "Items/ItemComponent.h"
#include "Items/ItemDefinition.h"
#include "Inventory/InventoryComponent.h"
#include "Equipment/EquipmentComponent.h"
#include "Interaction/InteractableComponent.h"
#include "GameFramework/Actor.h"

UItemComponent::UItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Quantity = 1;
}

void UItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-bind to a sibling Interactable Component for "Plug & Play" functionality
	if (AActor* OwnerActor = GetOwner())
	{
		if (!bSimulatePhysics)
		{
			if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
			{
				bSimulatePhysics = RootPrim->IsSimulatingPhysics();
			}
		}

		if (UInteractableComponent* SiblingInteractable = OwnerActor->FindComponentByClass<UInteractableComponent>())
		{
			SiblingInteractable->OnInteract.RemoveDynamic(this, &UItemComponent::HandleInteraction);
			SiblingInteractable->OnInteract.AddDynamic(this, &UItemComponent::HandleInteraction);
		}
	}
}

void UItemComponent::HandleInteraction(AActor* Interactor)
{
	if (!GetOwner()->HasAuthority() || !ItemDef || Quantity <= 0 || !Interactor)
	{
		return;
	}

	bool bHandled = false;

	// 1. Try to Auto-Equip first (if the item allows it)
	if (ItemDef->ShouldAutoEquip())
	{
		if (UEquipmentComponent* EquipmentComp = Interactor->FindComponentByClass<UEquipmentComponent>())
		{
			FItemInstance UnequippedItem;
			bHandled = EquipmentComp->EquipItemFromWorld(this, UnequippedItem);

			if (bHandled)
			{
				OnPickedUp.Broadcast(Interactor);
			}
		}
	}

	// 2. If it wasn't equipped (not auto-equip, or equipment full/failed), send to Backpack
	if (!bHandled)
	{
		if (UInventoryComponent* InvComp = Interactor->FindComponentByClass<UInventoryComponent>())
		{
			// TryAddItem will automatically destroy GetOwner() if it fits in the backpack
			bHandled = InvComp->TryAddItem(ItemDef, Quantity, GetOwner());
		}
	}

	// If bHandled is true, the item is now either attached to the player (Equipment) 
	// or destroyed/hidden (Backpack). We don't need to do anything else here!
}