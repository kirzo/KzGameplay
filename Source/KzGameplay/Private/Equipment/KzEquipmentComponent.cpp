// Copyright 2026 kirzo

#include "Equipment/KzEquipmentComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Items/Fragments/KzItemFragment_Storable.h"
#include "Items/KzItemComponent.h"
#include "Inventory/KzInventoryComponent.h"
#include "Misc/KzTransformSource.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

// =================================================================
// FAST ARRAY CALLBACKS (client-side)
// =================================================================

void FEquippedSlot::PostReplicatedAdd(const FKzEquipmentList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->HandleSlotAdded(*this);
	}
}

void FEquippedSlot::PostReplicatedChange(const FKzEquipmentList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->HandleSlotUpdated(*this);
	}
}

void FEquippedSlot::PreReplicatedRemove(const FKzEquipmentList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->ClearVisualForSlot(*this);
	}
}

// =================================================================
// COMPONENT
// =================================================================

UKzEquipmentComponent::UKzEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	EquipmentList.OwnerComponent = this;

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
	DOREPLIFETIME(UKzEquipmentComponent, EquipmentList);
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

	EquipmentList.Slots.Empty();

	TArray<FKzEquipmentSlotDefinition> AllSlots;
	Layout->GetAllSlotDefinitions(AllSlots);

	for (const FKzEquipmentSlotDefinition& SlotDef : AllSlots)
	{
		EquipmentList.Slots.Add(FEquippedSlot(SlotDef.SlotID));
	}

	EquipmentList.MarkArrayDirty();
}

void UKzEquipmentComponent::RefreshVisualForSlot(FEquippedSlot& Slot)
{
	// Drop any existing local visual first, so swaps recreate cleanly.
	ClearVisualForSlot(Slot);

	// Dedicated servers render nothing, so skip the cosmetic mesh there.
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!Slot.Instance.IsValid())
	{
		return;
	}

	const UKzItemFragment_Equippable* EquipFrag = Slot.Instance.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
	if (!EquipFrag || EquipFrag->EquipmentSpawnMode != EKzEquipmentSpawnMode::SpawnMesh)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (!OwnerMesh)
	{
		return;
	}

	FName AttachmentSocket = EquipFrag->SocketOverride;
	if (AttachmentSocket.IsNone() && DefaultLayout)
	{
		AttachmentSocket = DefaultLayout->GetSocketForSlot(Slot.SlotID);
	}

	UMeshComponent* NewMesh = nullptr;
	if (UStreamableRenderAsset* LoadedMesh = EquipFrag->EquipmentMesh.LoadSynchronous())
	{
		if (UStaticMesh* AsStaticMesh = Cast<UStaticMesh>(LoadedMesh))
		{
			UStaticMeshComponent* NewSMC = NewObject<UStaticMeshComponent>(GetOwner());
			NewSMC->SetStaticMesh(AsStaticMesh);
			NewMesh = NewSMC;
		}
		else if (USkeletalMesh* AsSkeletalMesh = Cast<USkeletalMesh>(LoadedMesh))
		{
			USkeletalMeshComponent* NewSKMC = NewObject<USkeletalMeshComponent>(GetOwner());
			NewSKMC->SetSkeletalMesh(AsSkeletalMesh);
			NewMesh = NewSKMC;
		}
	}

	if (NewMesh)
	{
		NewMesh->RegisterComponent();
		NewMesh->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachmentSocket);
		NewMesh->SetRelativeTransform(EquipFrag->AttachmentOffset);

		if (EquipFrag->bDisableCollisionOnEquip)
		{
			NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		Slot.Instance.SpawnedComponent = NewMesh;
	}
}

void UKzEquipmentComponent::ClearVisualForSlot(FEquippedSlot& Slot)
{
	if (Slot.Instance.SpawnedComponent)
	{
		Slot.Instance.SpawnedComponent->DestroyComponent();
		Slot.Instance.SpawnedComponent = nullptr;
	}
}

void UKzEquipmentComponent::HandleSlotAdded(FEquippedSlot& Slot)
{
	RefreshVisualForSlot(Slot);

	// Only announce non-empty slots on initial replication (late joiners); empty slots stay silent.
	if (Slot.Instance.IsValid())
	{
		OnItemEquipped.Broadcast(Slot.SlotID, Slot.Instance);
	}
}

void UKzEquipmentComponent::HandleSlotUpdated(FEquippedSlot& Slot)
{
	RefreshVisualForSlot(Slot);

	if (Slot.Instance.IsValid())
	{
		OnItemEquipped.Broadcast(Slot.SlotID, Slot.Instance);
	}
	else
	{
		OnItemUnequipped.Broadcast(Slot.SlotID, Slot.Instance);
	}
}

bool UKzEquipmentComponent::EquipItem(const FKzItemInstance& ItemToEquip, FKzItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !ItemToEquip.IsValid() || !DefaultLayout) return false;

	const UKzItemFragment_Equippable* EquipFrag = ItemToEquip.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
	if (!EquipFrag) return false; // Not equippable!

	FGameplayTag TargetSlot = DefaultLayout->ResolveSlotID(EquipFrag->TargetSlot);

	for (FEquippedSlot& Slot : EquipmentList.Slots)
	{
		if (Slot.SlotID == TargetSlot)
		{
			if (Slot.Instance.IsValid())
			{
				UnequipItem(TargetSlot, OutUnequippedItem);
			}

			// Assign the new item to the slot
			Slot.Instance = ItemToEquip;

			USkeletalMeshComponent* OwnerMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();

			FName AttachmentSocket = EquipFrag->SocketOverride;
			if (AttachmentSocket.IsNone())
			{
				AttachmentSocket = DefaultLayout->GetSocketForSlot(TargetSlot);
			}

			UKzItemComponent* ItemComp = nullptr;

			// MeshComponent (Cosmetics, simple items): the visual is a local component
			// created per-machine by RefreshVisualForSlot, not a replicated actor.
			if (EquipFrag->EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh)
			{
				// Destroy the world actor if the item came from the ground.
				if (AActor* PhysicalActor = Slot.Instance.SpawnedActor)
				{
					PhysicalActor->Destroy();
					Slot.Instance.SpawnedActor = nullptr;
				}
			}
			// Full Actor (Weapons, complex logic)
			else
			{
				TSubclassOf<AActor> TargetClass = EquipFrag->GetEquippedActorClass(ItemToEquip.ItemDef->WorldActorClass).LoadSynchronous();
				AActor* CurrentActor = Slot.Instance.SpawnedActor;

				// 1. If the actor from the ground is different from the equipped class, recreate it
				if (CurrentActor && TargetClass && CurrentActor->GetClass() != TargetClass)
				{
					CurrentActor->Destroy();
					CurrentActor = nullptr;
				}

				// 2. Spawn the equipped actor if we don't have one
				if (!CurrentActor && TargetClass)
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = GetOwner();
					SpawnParams.Instigator = Cast<APawn>(GetOwner());
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					CurrentActor = GetWorld()->SpawnActor<AActor>(TargetClass, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), SpawnParams);
					Slot.Instance.SpawnedActor = CurrentActor;
				}

				// 3. Attach the actor
				if (CurrentActor)
				{
					CurrentActor->SetOwner(GetOwner());
					CurrentActor->SetInstigator(Cast<APawn>(GetOwner()));

					ItemComp = CurrentActor->FindComponentByClass<UKzItemComponent>();

					if (UPrimitiveComponent* OwnerPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
					{
						OwnerPrim->IgnoreActorWhenMoving(CurrentActor, true);
					}

					if (EquipFrag->bDisableCollisionOnEquip)
					{
						CurrentActor->SetActorEnableCollision(false);
					}

					if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(CurrentActor->GetRootComponent()))
					{
						if (RootPrim->IsSimulatingPhysics())
						{
							RootPrim->SetSimulatePhysics(false);
						}
					}

					if (OwnerMesh)
					{
						if (EquipFrag->bUseCustomAttachment && ItemComp)
						{
							ItemComp->OnCustomAttach.Broadcast(GetOwner(), FKzTransformSource(OwnerMesh, AttachmentSocket, EquipFrag->AttachmentOffset));
						}
						else
						{
							CurrentActor->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachmentSocket);
							CurrentActor->SetActorRelativeTransform(EquipFrag->AttachmentOffset);
						}
					}

					if (ItemComp)
					{
						ItemComp->SetEquippedState(GetOwner(), TargetSlot);
					}
				}
			}

			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
			{
				for (const FGameplayTag& Tag : EquipFrag->EquippedTags)
				{
					ASC->AddLooseGameplayTag(Tag);
				}
			}

			Slot.Instance.ActiveEquippedAction = EquipFrag->OnEquippedAction.Clone(this);
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("Equipment"), this);
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("Item"), ItemComp);
			Slot.Instance.ActiveEquippedAction.SetContextProperty(TEXT("ItemActor"), ItemToEquip.SpawnedActor);
			Slot.Instance.ActiveEquippedAction.Run(this);

			OnItemEquippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
			OnItemEquippedAction.SetContextProperty(TEXT("Equipment"), this);
			OnItemEquippedAction.SetContextProperty(TEXT("ItemActor"), ItemToEquip.SpawnedActor);
			OnItemEquippedAction.SetContextProperty(TEXT("SlotID"), Slot.SlotID);
			OnItemEquippedAction.Run(this);

			// Create the authority machine's local visual and replicate the slot.
			RefreshVisualForSlot(Slot);
			EquipmentList.MarkItemDirty(Slot);

			// Notify server listeners
			OnItemEquipped.Broadcast(TargetSlot, ItemToEquip);

			return true;
		}
	}

	// Target slot not found in the character's layout
	return false;
}

bool UKzEquipmentComponent::EquipItemByDefinition(const UKzItemDefinition* ItemDef, FKzItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !ItemDef)
	{
		return false;
	}

	// Create a fresh instance from the definition
	FKzItemInstance NewInstance(ItemDef);
	return EquipItem(NewInstance, OutUnequippedItem);
}

bool UKzEquipmentComponent::EquipItemFromWorld(UKzItemComponent* ItemComp, FKzItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !ItemComp || !ItemComp->ItemInstance.ItemDef)
	{
		return false;
	}

	return EquipItem(ItemComp->ItemInstance, OutUnequippedItem);
}

bool UKzEquipmentComponent::UnequipItem(FGameplayTag SlotID, FKzItemInstance& OutUnequippedItem)
{
	if (!GetOwner()->HasAuthority() || !DefaultLayout)
	{
		return false;
	}

	FGameplayTag TargetSlot = DefaultLayout->ResolveSlotID(SlotID);

	for (FEquippedSlot& Slot : EquipmentList.Slots)
	{
		if (Slot.SlotID == TargetSlot && Slot.Instance.IsValid())
		{
			OutUnequippedItem = Slot.Instance;

			const UKzItemFragment_Equippable* EquipFrag = OutUnequippedItem.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
			const UKzItemFragment_Storable* StoreFrag = OutUnequippedItem.ItemDef->FindFragmentByClass<UKzItemFragment_Storable>();

			OutUnequippedItem.ActiveEquippedAction.Reset();
			OutUnequippedItem.ActiveEquippedAction = FScriptableAction();

			// Destroy the authority machine's local cosmetic mesh; clients drop theirs via the callback.
			ClearVisualForSlot(Slot);
			OutUnequippedItem.SpawnedComponent = nullptr;

			Slot.Instance = FKzItemInstance(); // Clear the slot
			EquipmentList.MarkItemDirty(Slot);

			if (EquipFrag)
			{
				if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
				{
					for (const FGameplayTag& Tag : EquipFrag->EquippedTags)
					{
						ASC->RemoveLooseGameplayTag(Tag);
					}
				}
			}

			bool bSentToInventory = false;

			// 1. Check if the item is allowed to go to the backpack
			if (StoreFrag)
			{
				if (UKzInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UKzInventoryComponent>())
				{
					// TryAddItem will destroy the physical actor if it successfully stores it
					bSentToInventory = InvComp->TryAddItem(OutUnequippedItem.ItemDef, OutUnequippedItem.Quantity, OutUnequippedItem.SpawnedActor);
				}
			}

			UKzItemComponent* ItemComp = nullptr;

			// 2. If it couldn't go to the inventory (EquipmentOnly, or inventory full) -> Drop it to the ground
			if (!bSentToInventory)
			{
				// SpawnMesh items have no world actor yet: spawn one to represent the drop.
				if (EquipFrag && EquipFrag->EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh)
				{
					TSubclassOf<AActor> WorldClass = OutUnequippedItem.ItemDef->WorldActorClass.LoadSynchronous();
					if (WorldClass)
					{
						FActorSpawnParameters SpawnParams;
						SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						OutUnequippedItem.SpawnedActor = GetWorld()->SpawnActor<AActor>(WorldClass, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), SpawnParams);
					}
				}

				if (AActor* OldPhysicalActor = OutUnequippedItem.SpawnedActor)
				{
					OldPhysicalActor->SetOwner(nullptr);
					OldPhysicalActor->SetInstigator(nullptr);

					ItemComp = OldPhysicalActor->FindComponentByClass<UKzItemComponent>();

					if (ItemComp)
					{
						ItemComp->ClearEquippedState();

						if (EquipFrag && EquipFrag->bUseCustomAttachment)
						{
							ItemComp->OnCustomDetach.Broadcast(GetOwner());
						}
					}

					if (!EquipFrag || !EquipFrag->bUseCustomAttachment)
					{
						OldPhysicalActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
					}

					// Safe drop: push the dropped item clear of the character so it doesn't
					// spawn inside the capsule. Custom-attachment items manage their own
					// placement, so we must not relocate them.
					if (!EquipFrag || !EquipFrag->bUseCustomAttachment)
					{
						AActor* OwnerActor = GetOwner();

						const float OwnerRadius = OwnerActor->GetSimpleCollisionRadius();

						FVector ItemOrigin, ItemExtent;
						OldPhysicalActor->GetActorBounds(true, ItemOrigin, ItemExtent);

						const float SafeDistance = OwnerRadius + ItemExtent.GetMax() + 5.0f;

						FVector OwnerLoc = OwnerActor->GetActorLocation();
						FVector ForwardDir = OwnerActor->GetActorForwardVector();

						// Keep the current Z so it falls naturally from the hand socket, but
						// push it away in the XY plane before re-enabling collisions.
						FVector DropLocation = OldPhysicalActor->GetActorLocation();
						DropLocation.X = OwnerLoc.X + (ForwardDir.X * SafeDistance);
						DropLocation.Y = OwnerLoc.Y + (ForwardDir.Y * SafeDistance);

						OldPhysicalActor->SetActorLocation(DropLocation, false, nullptr, ETeleportType::TeleportPhysics);
					}

					if (UPrimitiveComponent* OwnerPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
					{
						OwnerPrim->IgnoreActorWhenMoving(OldPhysicalActor, false);
					}

					if (EquipFrag && EquipFrag->bDisableCollisionOnEquip)
					{
						OldPhysicalActor->SetActorEnableCollision(true);
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
							RootPrim->AddTorqueInRadians(FVector::CrossProduct(FVector::UpVector, GetOwner()->GetVelocity()), NAME_None, true);
						}
					}
				}
			}

			OnItemUnequippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
			OnItemUnequippedAction.SetContextProperty(TEXT("Equipment"), this);
			OnItemUnequippedAction.SetContextProperty(TEXT("ItemActor"), OutUnequippedItem.SpawnedActor);
			OnItemUnequippedAction.SetContextProperty(TEXT("SlotID"), Slot.SlotID);
			OnItemUnequippedAction.Run(this);

			OnItemUnequipped.Broadcast(SlotID, OutUnequippedItem);
			return true;
		}
	}

	return false;
}

const FKzItemInstance* UKzEquipmentComponent::FindItemInSlot(FGameplayTag SlotID) const
{
	if (!DefaultLayout)
	{
		return nullptr;
	}

	FGameplayTag TargetSlot = DefaultLayout->ResolveSlotID(SlotID);

	if (const FEquippedSlot* FoundSlot = EquipmentList.Slots.FindByKey(TargetSlot))
	{
		return &FoundSlot->Instance;
	}

	return nullptr;
}

FKzItemInstance UKzEquipmentComponent::GetItemInSlot(FGameplayTag SlotID) const
{
	if (const FKzItemInstance* FoundItem = FindItemInSlot(SlotID))
	{
		return *FoundItem;
	}
	return FKzItemInstance();
}