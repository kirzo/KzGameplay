// Copyright 2026 kirzo

#include "Items/KzItemComponent.h"
#include "Items/KzItemDefinition.h"
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

bool UKzItemComponent::HandleInteraction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable)
{
	if (!GetOwner()->HasAuthority() || !ItemDef || Quantity <= 0 || !Interactor)
	{
		return false;
	}

	bool bHandled = false;

	AActor* InteractorActor = Interactor->GetOwner();

	// 1. Try to Auto-Equip first (if the item allows it)
	if (ItemDef->ShouldAutoEquip())
	{
		if (UKzEquipmentComponent* EquipmentComp = InteractorActor->FindComponentByClass<UKzEquipmentComponent>())
		{
			FKzItemInstance UnequippedItem;
			bHandled = EquipmentComp->EquipItemFromWorld(this, UnequippedItem);

			if (bHandled)
			{
				OnPickedUp.Broadcast(InteractorActor);
			}
		}
	}

	// 2. If it wasn't equipped (not auto-equip, or equipment full/failed), send to Backpack
	if (!bHandled)
	{
		if (UKzInventoryComponent* InvComp = InteractorActor->FindComponentByClass<UKzInventoryComponent>())
		{
			// TryAddItem will automatically destroy GetOwner() if it fits in the backpack
			bHandled = InvComp->TryAddItem(ItemDef, Quantity, GetOwner());
		}
	}

	return bHandled;
}