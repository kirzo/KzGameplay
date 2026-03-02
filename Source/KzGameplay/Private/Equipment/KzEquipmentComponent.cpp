// Copyright 2026 kirzo

#include "Equipment/KzEquipmentComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemComponent.h"
#include "Inventory/KzInventoryComponent.h"
#include "Misc/KzTransformSource.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

UKzEquipmentComponent::UKzEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UKzEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Unlike the Inventory, equipment is usually visible to everyone (COND_None).
	DOREPLIFETIME(UKzEquipmentComponent, EquippedSlots);
}

void UKzEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority() && DefaultLayout)
	{
		InitializeEquipment(DefaultLayout);
	}
}

void UKzEquipmentComponent::InitializeEquipment(const UKzEquipmentLayout* Layout)
{
	if (!Layout) return;

	EquippedSlots.Empty();

	TArray<FKzEquipmentSlotDefinition> AllSlots;
	Layout->GetAllSlotDefinitions(AllSlots);

	for (const FKzEquipmentSlotDefinition& SlotDef : AllSlots)
	{
		EquippedSlots.Add(FEquippedSlot(SlotDef.SlotID));
	}
}

void UKzEquipmentComponent::OnRep_EquippedSlots(const TArray<FEquippedSlot>& OldEquippedSlots)
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

bool UKzEquipmentComponent::EquipItem(const FKzItemInstance& ItemToEquip, FKzItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !ItemToEquip.IsValid() || !ItemToEquip.ItemDef->IsEquippable())
	{
		return false;
	}

	FGameplayTag TargetSlot = DefaultLayout->ResolveSlotID(ItemToEquip.ItemDef->TargetSlot);

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

			UKzItemComponent* ItemComp = nullptr;

			if (AActor* NewPhysicalActor = ItemToEquip.PhysicalActor)
			{
				ItemComp = NewPhysicalActor->FindComponentByClass<UKzItemComponent>();

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
						if (ItemComp)
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

				if (ItemComp)
				{
					ItemComp->SetEquippedState(GetOwner(), TargetSlot);
				}
			}

			Slot.Instance.ActiveEquippedAction = Slot.Instance.ItemDef->OnEquippedAction.Clone(this);
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("Equipment"), this);
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("Item"), ItemComp);
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("ItemActor"), ItemToEquip.PhysicalActor);
			FScriptableAction::RunAction(this, Slot.Instance.ActiveEquippedAction);

			// 5. Notify server listeners
			OnEquipmentChanged.Broadcast(TargetSlot, ItemToEquip);

			return true;
		}
	}

	// Target slot not found in the character's layout
	return false;
}

bool UKzEquipmentComponent::EquipItemFromWorld(UKzItemComponent* ItemComp, FKzItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !ItemComp || !ItemComp->ItemDef)
	{
		return false;
	}

	return EquipItem(ItemComp->ToKzItemInstance(), OutUnequippedItem);
}

bool UKzEquipmentComponent::UnequipItem(FGameplayTag SlotID, FKzItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority())
	{
		return false;
	}

	FGameplayTag TargetSlot = DefaultLayout->ResolveSlotID(SlotID);

	for (FEquippedSlot& Slot : EquippedSlots)
	{
		if (Slot.SlotID == TargetSlot && Slot.Instance.IsValid())
		{
			OutUnequippedItem = Slot.Instance;

			OutUnequippedItem.ActiveEquippedAction.Reset();
			OutUnequippedItem.ActiveEquippedAction.Unregister();
			OutUnequippedItem.ActiveEquippedAction = FScriptableAction();

			Slot.Instance = FKzItemInstance(); // Clear the slot

			bool bSentToInventory = false;

			// 1. Check if the item is allowed to go to the backpack
			if (OutUnequippedItem.ItemDef->StorageMode != EKzItemStorageMode::EquipmentOnly)
			{
				if (UKzInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UKzInventoryComponent>())
				{
					// TryAddItem will destroy the physical actor if it successfully stores it
					bSentToInventory = InvComp->TryAddItem(OutUnequippedItem.ItemDef, OutUnequippedItem.Quantity, OutUnequippedItem.PhysicalActor);
				}
			}

			UKzItemComponent* ItemComp = nullptr;

			// 2. If it couldn't go to the inventory (EquipmentOnly, or inventory full) -> Drop it to the ground
			if (!bSentToInventory)
			{
				if (AActor* OldPhysicalActor = OutUnequippedItem.PhysicalActor)
				{
					ItemComp = OldPhysicalActor->FindComponentByClass<UKzItemComponent>();

					if (ItemComp)
					{
						ItemComp->ClearEquippedState();

						if (OutUnequippedItem.ItemDef->bUseCustomAttachment)
						{
							ItemComp->OnCustomDetach.Broadcast(GetOwner());
						}
					}

					if (!OutUnequippedItem.ItemDef->bUseCustomAttachment)
					{
						OldPhysicalActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
					}

					// ==========================================
					// Safe drop
					AActor* OwnerActor = GetOwner();

					// 1. Get Character's capsule radius
					const float OwnerRadius = OwnerActor->GetSimpleCollisionRadius();

					// 2. Get Item's bounding box (true = only collidable components)
					FVector ItemOrigin, ItemExtent;
					OldPhysicalActor->GetActorBounds(true, ItemOrigin, ItemExtent);

					// 3. Calculate safe push distance (Owner radius + Max Item Size + 5cm margin)
					const float SafeDistance = OwnerRadius + ItemExtent.GetMax() + 5.0f;

					// 4. Calculate the drop location in front of the character
					FVector OwnerLoc = OwnerActor->GetActorLocation();
					FVector ForwardDir = OwnerActor->GetActorForwardVector();

					// We keep the current Z (height) so it falls naturally from the hand socket, 
					// but we push it away in the XY plane.
					FVector DropLocation = OldPhysicalActor->GetActorLocation();
					DropLocation.X = OwnerLoc.X + (ForwardDir.X * SafeDistance);
					DropLocation.Y = OwnerLoc.Y + (ForwardDir.Y * SafeDistance);

					// Teleport the item to the safe spot BEFORE re-enabling collisions
					OldPhysicalActor->SetActorLocation(DropLocation, false, nullptr, ETeleportType::TeleportPhysics);
					// ==========================================

					if (UPrimitiveComponent* OwnerPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
					{
						OwnerPrim->IgnoreActorWhenMoving(OldPhysicalActor, false);
					}

					// Enable physics only if the KzItemComponent dictates it
					if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(OldPhysicalActor->GetRootComponent()))
					{
						bool bSimulatePhysics = false;
						if (ItemComp)
						{
							if (ItemComp->bSimulatePhysics)
							{
								bSimulatePhysics = true;
							}
						}
						else
						{
							bSimulatePhysics = true;
						}

						if (bSimulatePhysics)
						{
							RootPrim->SetSimulatePhysics(true);
							RootPrim->SetPhysicsLinearVelocity(GetOwner()->GetVelocity());
						}
					}
				}
			}

			OnEquipmentChanged.Broadcast(SlotID, FKzItemInstance());
			return true;
		}
	}

	return false;
}

FKzItemInstance UKzEquipmentComponent::GetItemInSlot(FGameplayTag SlotID) const
{
	FGameplayTag TargetSlot = DefaultLayout->ResolveSlotID(SlotID);

	for (const FEquippedSlot& Slot : EquippedSlots)
	{
		if (Slot.SlotID == TargetSlot)
		{
			return Slot.Instance;
		}
	}

	return FKzItemInstance();
}