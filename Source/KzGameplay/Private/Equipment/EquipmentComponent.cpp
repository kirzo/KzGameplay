// Copyright 2026 kirzo

#include "Equipment/EquipmentComponent.h"
#include "Items/ItemDefinition.h"
#include "Items/ItemComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Misc/KzTransformSource.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Unlike the Inventory, equipment is usually visible to everyone (COND_None).
	DOREPLIFETIME(UEquipmentComponent, EquippedSlots);
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority() && DefaultLayout)
	{
		InitializeEquipment(DefaultLayout);
	}
}

void UEquipmentComponent::InitializeEquipment(const UEquipmentLayout* Layout)
{
	if (!Layout) return;

	EquippedSlots.Empty();

	TArray<FEquipmentSlotDefinition> AllSlots;
	Layout->GetAllSlotDefinitions(AllSlots);

	for (const FEquipmentSlotDefinition& SlotDef : AllSlots)
	{
		EquippedSlots.Add(FEquippedSlot(SlotDef.SlotID));
	}
}

void UEquipmentComponent::OnRep_EquippedSlots(const TArray<FEquippedSlot>& OldEquippedSlots)
{
	// Iterate through the new state of the equipped slots
	for (const FEquippedSlot& NewSlot : EquippedSlots)
	{
		// Find the corresponding slot in the old state
		const FEquippedSlot* OldSlotPtr = OldEquippedSlots.FindByPredicate([&NewSlot](const FEquippedSlot& OldSlot)
			{
				return OldSlot.SlotID == NewSlot.SlotID;
			});

		bool bSlotChanged = false;

		if (!OldSlotPtr)
		{
			// Rare case: a new slot was added at runtime
			bSlotChanged = true;
		}
		else
		{
			// Check if the physical actor or the item definition has changed
			if (OldSlotPtr->Instance.PhysicalActor != NewSlot.Instance.PhysicalActor ||
				OldSlotPtr->Instance.ItemDef != NewSlot.Instance.ItemDef)
			{
				bSlotChanged = true;
			}
		}

		// If a change is detected in this specific slot, broadcast the precise event
		if (bSlotChanged)
		{
			// Notify UI and AnimBP (e.g., to seamlessly swap to a "Holding Weapon" pose)
			OnEquipmentChanged.Broadcast(NewSlot.SlotID, NewSlot.Instance);
		}
	}
}

bool UEquipmentComponent::EquipItem(const FItemInstance& ItemToEquip, FItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !ItemToEquip.IsValid() || !ItemToEquip.ItemDef->IsEquippable())
	{
		return false;
	}

	FGameplayTag TargetSlot = ItemToEquip.ItemDef->TargetSlot;

	for (FEquippedSlot& Slot : EquippedSlots)
	{
		if (Slot.SlotID == TargetSlot)
		{
			if (Slot.Instance.IsValid())
			{
				UnequipItem(TargetSlot, OutUnequippedItem);
			}
			
			// Assign the new item to the slot
			Slot.Instance = ItemToEquip;

			if (AActor* NewPhysicalActor = ItemToEquip.PhysicalActor)
			{
				if (UPrimitiveComponent* OwnerPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
				{
					OwnerPrim->IgnoreActorWhenMoving(NewPhysicalActor, true);
				}

				if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(NewPhysicalActor->GetRootComponent()))
				{
					if (RootPrim->IsSimulatingPhysics())
					{
						RootPrim->SetSimulatePhysics(false);
					}
				}

				if (USkeletalMeshComponent* MeshComp = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
				{
					FName AttachmentSocket = DefaultLayout->GetSocketForSlot(TargetSlot);

					if (ItemToEquip.ItemDef->bUseCustomAttachment)
					{
						if (UItemComponent* ItemComp = NewPhysicalActor->FindComponentByClass<UItemComponent>())
						{
							ItemComp->OnCustomAttach.Broadcast(GetOwner(), FKzTransformSource(MeshComp, AttachmentSocket, ItemToEquip.ItemDef->AttachmentOffset));
						}
					}
					else
					{
						NewPhysicalActor->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachmentSocket);
						NewPhysicalActor->SetActorRelativeTransform(ItemToEquip.ItemDef->AttachmentOffset);
					}
				}
			}

			// TODO: Execute Scriptable Framework OnEquippedAction

			// 5. Notify server listeners
			OnEquipmentChanged.Broadcast(TargetSlot, ItemToEquip);

			return true;
		}
	}

	// Target slot not found in the character's layout
	return false;
}

bool UEquipmentComponent::EquipItemFromWorld(UItemComponent* ItemComp, FItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !ItemComp || !ItemComp->ItemDef)
	{
		return false;
	}

	return EquipItem(ItemComp->ToItemInstance(), OutUnequippedItem);
}

bool UEquipmentComponent::UnequipItem(FGameplayTag SlotID, FItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	for (FEquippedSlot& Slot : EquippedSlots)
	{
		if (Slot.SlotID == SlotID && Slot.Instance.IsValid())
		{
			OutUnequippedItem = Slot.Instance;
			Slot.Instance = FItemInstance(); // Clear the slot

			bool bSentToInventory = false;

			// 1. Check if the item is allowed to go to the backpack
			if (OutUnequippedItem.ItemDef->StorageMode != EKzItemStorageMode::EquipmentOnly)
			{
				if (UInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UInventoryComponent>())
				{
					// TryAddItem will destroy the physical actor if it successfully stores it
					bSentToInventory = InvComp->TryAddItem(OutUnequippedItem.ItemDef, OutUnequippedItem.Quantity, OutUnequippedItem.PhysicalActor);
				}
			}

			// 2. If it couldn't go to the inventory (EquipmentOnly, or inventory full) -> Drop it to the ground
			if (!bSentToInventory)
			{
				if (AActor* OldPhysicalActor = OutUnequippedItem.PhysicalActor)
				{
					if (OutUnequippedItem.ItemDef->bUseCustomAttachment)
					{
						if (UItemComponent* ItemComp = OldPhysicalActor->FindComponentByClass<UItemComponent>())
						{
							ItemComp->OnCustomDetach.Broadcast(GetOwner());
						}
					}
					else
					{
						OldPhysicalActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
					}

					if (UPrimitiveComponent* OwnerPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
					{
						OwnerPrim->IgnoreActorWhenMoving(OldPhysicalActor, false);
					}

					// Enable physics only if the ItemComponent dictates it
					if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(OldPhysicalActor->GetRootComponent()))
					{
						if (UItemComponent* ItemComp = OldPhysicalActor->FindComponentByClass<UItemComponent>())
						{
							if (ItemComp->bSimulatePhysics)
							{
								RootPrim->SetSimulatePhysics(true);
							}
						}
						else
						{
							RootPrim->SetSimulatePhysics(true);
						}
					}
				}
			}

			// TODO: Execute Scriptable Framework OnUnequippedAction for OutUnequippedItem

			OnEquipmentChanged.Broadcast(SlotID, FItemInstance());
			return true;
		}
	}

	return false;
}

FItemInstance UEquipmentComponent::GetItemInSlot(FGameplayTag SlotID) const
{
	for (const FEquippedSlot& Slot : EquippedSlots)
	{
		if (Slot.SlotID == SlotID)
		{
			return Slot.Instance;
		}
	}

	return FItemInstance();
}