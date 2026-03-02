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

	OnItemEquippedAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	OnItemEquippedAction.AddContextProperty<UKzEquipmentComponent*>(TEXT("Equipment"));
	OnItemEquippedAction.AddContextProperty<AActor*>(TEXT("ItemActor"));
	OnItemEquippedAction.AddContextProperty<FGameplayTag>(TEXT("SlotID"));

	OnItemUnequippedAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	OnItemUnequippedAction.AddContextProperty<UKzEquipmentComponent*>(TEXT("Equipment"));
	OnItemUnequippedAction.AddContextProperty<AActor*>(TEXT("ItemActor"));
	OnItemUnequippedAction.AddContextProperty<FGameplayTag>(TEXT("SlotID"));
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
	for (const FEquippedSlot& NewSlot : EquippedSlots)
	{
		const FEquippedSlot* OldSlotPtr = OldEquippedSlots.FindByPredicate([&NewSlot](const FEquippedSlot& OldSlot)
			{
				return OldSlot.SlotID == NewSlot.SlotID;
			});

		// If it's a completely new slot (rare) or the item inside has changed
		if (!OldSlotPtr || OldSlotPtr->Instance.PhysicalActor != NewSlot.Instance.PhysicalActor || OldSlotPtr->Instance.ItemDef != NewSlot.Instance.ItemDef)
		{
			// 1. If there was an old item, it means we dropped or replaced it
			if (OldSlotPtr && OldSlotPtr->Instance.IsValid())
			{
				OnItemUnequipped.Broadcast(NewSlot.SlotID, OldSlotPtr->Instance);
			}

			// 2. If there is a new item in the slot, we just equipped it
			if (NewSlot.Instance.IsValid())
			{
				OnItemEquipped.Broadcast(NewSlot.SlotID, NewSlot.Instance);
			}
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
			Slot.Instance.ActiveEquippedAction.Run(this);

			OnItemEquippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
			OnItemEquippedAction.SetContextProperty(TEXT("Equipment"), this);
			OnItemEquippedAction.SetContextProperty(TEXT("ItemActor"), ItemToEquip.PhysicalActor);
			OnItemEquippedAction.SetContextProperty(TEXT("SlotID"), Slot.SlotID);
			OnItemEquippedAction.Run(this);

			// 5. Notify server listeners
			OnItemEquipped.Broadcast(TargetSlot, ItemToEquip);

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

			OnItemUnequippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
			OnItemUnequippedAction.SetContextProperty(TEXT("Equipment"), this);
			OnItemUnequippedAction.SetContextProperty(TEXT("ItemActor"), OutUnequippedItem.PhysicalActor);
			OnItemUnequippedAction.SetContextProperty(TEXT("SlotID"), Slot.SlotID);
			OnItemUnequippedAction.Run(this);

			OnItemUnequipped.Broadcast(SlotID, OutUnequippedItem);
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