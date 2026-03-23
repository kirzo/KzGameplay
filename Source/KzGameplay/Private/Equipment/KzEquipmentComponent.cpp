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
		if (!OldSlotPtr || OldSlotPtr->Instance.SpawnedActor != NewSlot.Instance.SpawnedActor || OldSlotPtr->Instance.ItemDef != NewSlot.Instance.ItemDef)
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
	if (!GetOwner()->HasAuthority() || !ItemToEquip.IsValid()) return false;

	const UKzItemFragment_Equippable* EquipFrag = ItemToEquip.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
	if (!EquipFrag) return false; // Not equippable!

	FGameplayTag TargetSlot = DefaultLayout->ResolveSlotID(EquipFrag->TargetSlot);

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

			USkeletalMeshComponent* OwnerMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();

			FName AttachmentSocket = EquipFrag->SocketOverride;
			if (AttachmentSocket.IsNone())
			{
				AttachmentSocket = DefaultLayout->GetSocketForSlot(TargetSlot);
			}

			UKzItemComponent* ItemComp = nullptr;

			// MeshComponent (Cosmetics, simple items)
			if (EquipFrag->EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh)
			{
				// 1. Destroy the world actor if the item came from the ground
				if (AActor* PhysicalActor = Slot.Instance.SpawnedActor)
				{
					PhysicalActor->Destroy();
					Slot.Instance.SpawnedActor = nullptr;
				}

				// 2. Create the proper Mesh Component
				if (UStreamableRenderAsset* LoadedMesh = EquipFrag->EquipmentMesh.LoadSynchronous())
				{
					if (UStaticMesh* AsStaticMesh = Cast<UStaticMesh>(LoadedMesh))
					{
						UStaticMeshComponent* NewSMC = NewObject<UStaticMeshComponent>(GetOwner());
						NewSMC->SetStaticMesh(AsStaticMesh);
						Slot.Instance.SpawnedComponent = NewSMC;
					}
					else if (USkeletalMesh* AsSkeletalMesh = Cast<USkeletalMesh>(LoadedMesh))
					{
						USkeletalMeshComponent* NewSKMC = NewObject<USkeletalMeshComponent>(GetOwner());
						NewSKMC->SetSkeletalMesh(AsSkeletalMesh);
						Slot.Instance.SpawnedComponent = NewSKMC;
					}

					// 3. Attach the new component to the character
					if (Slot.Instance.SpawnedComponent && OwnerMesh)
					{
						Slot.Instance.SpawnedComponent->RegisterComponent();
						Slot.Instance.SpawnedComponent->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachmentSocket);
						Slot.Instance.SpawnedComponent->SetRelativeTransform(EquipFrag->AttachmentOffset);

						if (EquipFrag->bDisableCollisionOnEquip)
						{
							Slot.Instance.SpawnedComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						}
					}
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

			// 5. Notify server listeners
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

			const UKzItemFragment_Equippable* EquipFrag = OutUnequippedItem.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
			const UKzItemFragment_Storable* StoreFrag = OutUnequippedItem.ItemDef->FindFragmentByClass<UKzItemFragment_Storable>();

			OutUnequippedItem.ActiveEquippedAction.Reset();
			OutUnequippedItem.ActiveEquippedAction = FScriptableAction();

			Slot.Instance = FKzItemInstance(); // Clear the slot

			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
			{
				for (const FGameplayTag& Tag : EquipFrag->EquippedTags)
				{
					ASC->RemoveLooseGameplayTag(Tag);
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
				// Handle Component cleanup if it was a Mesh Spawn
				if (EquipFrag->EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh)
				{
					if (OutUnequippedItem.SpawnedComponent)
					{
						OutUnequippedItem.SpawnedComponent->DestroyComponent();
						OutUnequippedItem.SpawnedComponent = nullptr;
					}

					// Spawn the World Actor representation to drop on the ground
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

					if (EquipFrag->bDisableCollisionOnEquip)
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

	if (const FEquippedSlot* FoundSlot = EquippedSlots.FindByKey(TargetSlot))
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