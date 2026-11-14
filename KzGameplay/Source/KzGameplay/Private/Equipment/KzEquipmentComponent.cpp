// Copyright 2026 kirzo

#include "Equipment/KzEquipmentComponent.h"
#include "Equipment/KzEquipListener.h"
#include "Items/KzItemDefinition.h"
#include "Capabilities/KzCapabilityComponent.h"
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
		InArraySerializer.OwnerComponent->HandleSlotRemoved(*this);
	}
}

namespace
{
	const UKzItemDefinition* GetHeldItem(const FEquippedSlot& Slot)
	{
		return Slot.Instance.IsValid() ? Slot.Instance.ItemDef.Get() : nullptr;
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

		for (const UKzItemDefinition* Item : DefaultItems)
		{
			FKzItemInstance Unequipped;
			EquipItemByDefinition(Item, Unequipped);
		}
	}
}

void UKzEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (FEquippedSlot& Slot : EquipmentList.Slots)
	{
		ReleaseItemActor(Slot);
	}

	Super::EndPlay(EndPlayReason);
}

void UKzEquipmentComponent::InitializeEquipment(const UKzEquipmentLayout* Layout)
{
	if (!Layout) return;

	ActiveLayout = Layout;
	EquipmentList.Slots.Empty();

	TArray<FKzEquipmentSlotDefinition> AllSlots;
	Layout->GetAllSlotDefinitions(AllSlots);

	for (const FKzEquipmentSlotDefinition& SlotDef : AllSlots)
	{
		EquipmentList.Slots.Add(FEquippedSlot(SlotDef.SlotID));
	}

	EquipmentList.MarkArrayDirty();
}

const UKzEquipmentLayout* UKzEquipmentComponent::GetLayout() const
{
	// Clients never run InitializeEquipment, so their slots take their rules from the default layout.
	return ActiveLayout ? ActiveLayout.Get() : DefaultLayout.Get();
}

USceneComponent* UKzEquipmentComponent::GetAttachParentForSlot(FGameplayTag SlotID) const
{
	FKzEquipmentSlotDefinition Definition;
	if (const UKzEquipmentLayout* Layout = GetLayout())
	{
		if (Layout->FindSlotDefinition(SlotID, Definition) && !Definition.AttachComponent.ComponentName.IsNone())
		{
			return Definition.AttachComponent.GetComponent(this);
		}
	}

	// A slot that names no component means a bone socket on the character.
	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

USceneComponent* UKzEquipmentComponent::FindAttachmentForItem(const UKzItemFragment_Equippable& Equippable, FName& OutSocket) const
{
	const UKzEquipmentLayout* Layout = GetLayout();
	if (!Layout)
	{
		return nullptr;
	}

	const FGameplayTag SlotID = Layout->ResolveSlotID(Equippable.TargetSlot);
	OutSocket = Equippable.SocketOverride.IsNone() ? Layout->GetSocketForSlot(SlotID) : Equippable.SocketOverride;
	return GetAttachParentForSlot(SlotID);
}

USceneComponent* UKzEquipmentComponent::GetAttachmentForSlot(const FEquippedSlot& Slot, const UKzItemFragment_Equippable& Equippable, FName& OutSocket, FTransform& OutOffset) const
{
	const UKzEquipmentLayout* Layout = GetLayout();
	if (!Layout)
	{
		return nullptr;
	}

	// The item's own socket is for its Target Slot: put away, or in an Extra Slot, it hangs from that slot's
	const bool bStowed = IsStowed(Slot, Equippable);
	const bool bOwnSocket = !bStowed && !Equippable.SocketOverride.IsNone() && Slot.SlotID == Layout->ResolveSlotID(Equippable.TargetSlot);
	OutSocket = bOwnSocket ? Equippable.SocketOverride : Layout->GetSocketForSlot(Slot.SlotID);
	OutOffset = bStowed ? Equippable.StowedOffset : Equippable.AttachmentOffset;
	return GetAttachParentForSlot(Slot.SlotID);
}

FGameplayTag UKzEquipmentComponent::FindSlotFor(const UKzItemDefinition* ItemDef, const AActor* ItemActor) const
{
	const UKzEquipmentLayout* Layout = GetLayout();
	const UKzItemFragment_Equippable* Equippable = ItemDef ? ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
	if (!Layout || !Equippable)
	{
		return FGameplayTag();
	}

	TArray<FGameplayTag> Candidates = { Equippable->TargetSlot };
	Candidates.Append(Equippable->ExtraSlots);
	for (const FGameplayTag& Candidate : Candidates)
	{
		const FEquippedSlot* Slot = EquipmentList.Slots.FindByKey(Layout->ResolveSlotID(Candidate));
		if (Slot && !(ItemActor && Slot->Instance.SpawnedActor.Get() == ItemActor))
		{
			return Slot->SlotID;
		}
	}

	return FGameplayTag();
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
	if (!EquipFrag)
	{
		return;
	}

	// An actor shows itself, unless put away behind a stowed mesh of its own
	const bool bStowed = IsStowed(Slot, *EquipFrag);
	const bool bStowedMesh = bStowed && !EquipFrag->StowedMesh.IsNull();
	if (EquipFrag->EquipmentSpawnMode != EKzEquipmentSpawnMode::SpawnMesh && !bStowedMesh)
	{
		return;
	}

	FName AttachmentSocket;
	FTransform AttachmentOffset;
	USceneComponent* AttachParent = GetAttachmentForSlot(Slot, *EquipFrag, AttachmentSocket, AttachmentOffset);
	if (!AttachParent)
	{
		return;
	}

	UMeshComponent* NewMesh = nullptr;
	if (UStreamableRenderAsset* LoadedMesh = (bStowedMesh ? EquipFrag->StowedMesh : EquipFrag->EquipmentMesh).LoadSynchronous())
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
		NewMesh->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachmentSocket);
		NewMesh->SetRelativeTransform(AttachmentOffset);

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

void UKzEquipmentComponent::ShowItem(FEquippedSlot& Slot, const UKzItemDefinition* ItemDef)
{
	// Put away, it is out of use: its listeners hear it went, as a pose for holding it would
	const UKzItemFragment_Equippable* StowedEquippable = ItemDef ? ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
	if (StowedEquippable && IsStowed(Slot, *StowedEquippable))
	{
		ItemDef = nullptr;
	}

	// Listeners are cosmetic, like the mesh. A replicated change that keeps the item, such as a stat, tells them nothing.
	if (Slot.ShownItem == ItemDef || (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer))
	{
		return;
	}

	const UKzItemDefinition* PutAway = Slot.ShownItem;
	Slot.ShownItem = ItemDef;

	FKzEquipEvent Event;
	Event.Owner = GetOwner();
	Event.Equipment = this;
	Event.SlotID = Slot.SlotID;

	if (const UKzItemFragment_Equippable* Equippable = PutAway ? PutAway->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr)
	{
		Event.Item = PutAway;
		Equippable->NotifyUnequipped(Event);
	}

	if (const UKzItemFragment_Equippable* Equippable = ItemDef ? ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr)
	{
		Event.Item = ItemDef;
		Equippable->NotifyEquipped(Event);
	}
}

void UKzEquipmentComponent::HoldItemActor(FEquippedSlot& Slot)
{
	AActor* ItemActor = Slot.Instance.IsValid() ? Slot.Instance.SpawnedActor.Get() : nullptr;
	if (Slot.HeldActor.Get() == ItemActor)
	{
		return;
	}

	ReleaseItemActor(Slot);

	const UKzItemFragment_Equippable* Equippable = ItemActor ? Slot.Instance.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
	if (!Equippable)
	{
		return;
	}
	Slot.HeldActor = ItemActor;

	FName Socket;
	FTransform Offset;
	USceneComponent* AttachParent = GetAttachmentForSlot(Slot, *Equippable, Socket, Offset);

	// Out of the world while held: no physics, no collision if it asks for none, and its holder walks through it
	ItemActor->SetActorHiddenInGame(IsStowed(Slot, *Equippable) && !Equippable->StowedMesh.IsNull());
	if (Equippable->bDisableCollisionOnEquip)
	{
		ItemActor->SetActorEnableCollision(false);
	}
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(ItemActor->GetRootComponent()))
	{
		RootPrim->SetSimulatePhysics(false);
	}
	if (UPrimitiveComponent* OwnerPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
	{
		OwnerPrim->IgnoreActorWhenMoving(ItemActor, true);
	}

	if (AttachParent && !Equippable->bUseCustomAttachment)
	{
		ItemActor->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
		ItemActor->SetActorRelativeTransform(Offset);
	}

	if (UKzItemComponent* ItemComp = ItemActor->FindComponentByClass<UKzItemComponent>())
	{
		FKzItemHolder Holder;
		Holder.Equipment = this;
		Holder.SlotID = Slot.SlotID;
		Holder.PickedUpBy = Slot.PickedUpBy;
		ItemComp->AddHolder(Holder, FKzTransformSource(AttachParent, Socket, Offset));
	}
}

void UKzEquipmentComponent::ReleaseItemActor(FEquippedSlot& Slot)
{
	AActor* ItemActor = Slot.HeldActor.Get();
	Slot.HeldActor.Reset();
	if (!ItemActor)
	{
		return;
	}

	if (ItemActor->GetAttachParentActor() == GetOwner())
	{
		ItemActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	UKzItemComponent* ItemComp = ItemActor->FindComponentByClass<UKzItemComponent>();
	if (ItemComp)
	{
		ItemComp->RemoveHolder(this, Slot.SlotID);
	}

	// Its holder goes on walking through it while another of its slots holds it, as the other end of a rope
	if (!ItemComp || !ItemComp->IsHeldBy(GetOwner()))
	{
		if (UPrimitiveComponent* OwnerPrim = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
		{
			OwnerPrim->IgnoreActorWhenMoving(ItemActor, false);
		}
	}

	if (!ItemComp || !ItemComp->IsHeld())
	{
		ItemActor->SetActorHiddenInGame(false);
		ItemActor->SetActorEnableCollision(true);
		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(ItemActor->GetRootComponent()))
		{
			RootPrim->SetSimulatePhysics(ItemComp ? ItemComp->bSimulatePhysics : true);
		}
	}
}

void UKzEquipmentComponent::RefreshEquippedGrants()
{
	if (!GetOwner())
	{
		return;
	}

	// Tags do not need one, so a character without capabilities still gets its State.Grabbing
	UKzCapabilityComponent* Capabilities = GetOwner()->FindComponentByClass<UKzCapabilityComponent>();

	const UKzEquipmentLayout* Layout = GetLayout();

	TSet<FGameplayTag> Wanted;
	TSet<FGameplayTag> WantedTags;

	for (const FEquippedSlot& Slot : EquipmentList.Slots)
	{
		if (!Slot.Instance.IsValid())
		{
			continue;
		}

		// What the hand grants for holding anything, whatever it is holding
		FKzEquipmentSlotDefinition Definition;
		if (Layout && Layout->FindSlotDefinition(Slot.SlotID, Definition))
		{
			for (const FGameplayTag& Tag : Definition.GrantedCapabilities)
			{
				Wanted.Add(Tag);
			}

			for (const FGameplayTag& Tag : Definition.EquippedTags)
			{
				WantedTags.Add(Tag);
			}

			// And what the slot grants for the kind of thing it is holding
			for (const TPair<TSubclassOf<UKzItemFragment>, FGameplayTagContainer>& Pair : Definition.FragmentCapabilities)
			{
				if (Pair.Key && Slot.Instance.ItemDef && Slot.Instance.ItemDef->HasFragment(Pair.Key))
				{
					for (const FGameplayTag& Tag : Pair.Value)
					{
						Wanted.Add(Tag);
					}
				}
			}
		}

		// And what this particular object grants on top, unless it is put away
		const UKzItemFragment_Equippable* Equippable = Slot.Instance.ItemDef ? Slot.Instance.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
		if (Slot.Instance.ItemDef && !(Equippable && IsStowed(Slot, *Equippable)))
		{
			if (const UKzItemFragment_Equippable* Fragment = Equippable)
			{
				for (const FGameplayTag& Tag : Fragment->GrantedCapabilities)
				{
					Wanted.Add(Tag);
				}

				for (const FGameplayTag& Tag : Fragment->EquippedTags)
				{
					WantedTags.Add(Tag);
				}
			}

			// And what its fragments grant on their own
			FGameplayTagContainer FragmentTags;
			for (const UKzItemFragment* Fragment : Slot.Instance.ItemDef->Fragments)
			{
				if (Fragment)
				{
					Fragment->GetEquippedTags(FragmentTags);
				}
			}
			for (const FGameplayTag& Tag : FragmentTags)
			{
				WantedTags.Add(Tag);
			}
		}
	}

	if (Capabilities)
	{
		for (const FGameplayTag& Gone : ContributedCapabilities.Difference(Wanted))
		{
			Capabilities->RevokeCapability(Gone);
		}

		for (const FGameplayTag& Added : Wanted.Difference(ContributedCapabilities))
		{
			Capabilities->GrantCapability(Added);
		}

		ContributedCapabilities = MoveTemp(Wanted);
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		for (const FGameplayTag& Gone : ContributedTags.Difference(WantedTags))
		{
			ASC->RemoveLooseGameplayTag(Gone);
		}

		for (const FGameplayTag& Added : WantedTags.Difference(ContributedTags))
		{
			ASC->AddLooseGameplayTag(Added);
		}

		ContributedTags = MoveTemp(WantedTags);
	}
}

void UKzEquipmentComponent::HandleSlotAdded(FEquippedSlot& Slot)
{
	HoldItemActor(Slot);
	RefreshVisualForSlot(Slot);
	ShowItem(Slot, GetHeldItem(Slot));
	RefreshEquippedGrants();

	// Only announce non-empty slots on initial replication (late joiners); empty slots stay silent.
	if (Slot.Instance.IsValid())
	{
		OnItemEquipped.Broadcast(Slot.SlotID, Slot.Instance);
	}
}

void UKzEquipmentComponent::HandleSlotUpdated(FEquippedSlot& Slot)
{
	HoldItemActor(Slot);
	RefreshVisualForSlot(Slot);
	ShowItem(Slot, GetHeldItem(Slot));
	RefreshEquippedGrants();

	if (Slot.Instance.IsValid())
	{
		OnItemEquipped.Broadcast(Slot.SlotID, Slot.Instance);
	}
	else
	{
		OnItemUnequipped.Broadcast(Slot.SlotID, Slot.Instance);
	}
}

void UKzEquipmentComponent::HandleSlotRemoved(FEquippedSlot& Slot)
{
	ReleaseItemActor(Slot);
	ClearVisualForSlot(Slot);
	ShowItem(Slot, nullptr);
}

bool UKzEquipmentComponent::EquipItem(const FKzItemInstance& ItemToEquip, FKzItemInstance& OutUnequippedItem, UKzInteractableComponent* PickedUpBy)
{
	const UKzEquipmentLayout* Layout = GetLayout();
	if (!GetOwner()->HasAuthority() || !ItemToEquip.IsValid() || !Layout) return false;

	const UKzItemFragment_Equippable* EquipFrag = ItemToEquip.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
	if (!EquipFrag) return false; // Not equippable!

	// Held already, by this equipment or by others: one more slot takes it only while there is room for it
	AActor* WorldActor = ItemToEquip.SpawnedActor;
	const UKzItemComponent* WorldItem = WorldActor ? WorldActor->FindComponentByClass<UKzItemComponent>() : nullptr;
	if (WorldItem && !WorldItem->HasRoomForHolder())
	{
		return false;
	}

	// Target slot not found in the character's layout, or every slot it may go to holds it already
	const FGameplayTag TargetSlot = FindSlotFor(ItemToEquip.ItemDef, WorldActor);
	FEquippedSlot* Slot = EquipmentList.Slots.FindByKey(TargetSlot);
	if (!Slot)
	{
		return false;
	}

	if (Slot->Instance.IsValid())
	{
		UnequipItem(TargetSlot, OutUnequippedItem);
	}

	// Assign the new item to the slot
	Slot->Instance = ItemToEquip;
	Slot->PickedUpBy = PickedUpBy;

	// MeshComponent (Cosmetics, simple items): the visual is a local component
	// created per-machine by RefreshVisualForSlot, not a replicated actor.
	if (EquipFrag->EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh)
	{
		// Destroy the world actor if the item came from the ground.
		if (AActor* PhysicalActor = Slot->Instance.SpawnedActor)
		{
			PhysicalActor->Destroy();
			Slot->Instance.SpawnedActor = nullptr;
		}
	}
	// Full Actor (Weapons, complex logic)
	else
	{
		TSubclassOf<AActor> TargetClass = EquipFrag->GetEquippedActorClass(ItemToEquip.ItemDef->WorldActorClass).LoadSynchronous();
		AActor* CurrentActor = Slot->Instance.SpawnedActor;

		// 1. If the actor from the ground is not of the equipped class, recreate it. A subclass is, and keeps its own setup
		if (CurrentActor && TargetClass && !CurrentActor->IsA(TargetClass))
		{
			CurrentActor->Destroy();
			CurrentActor = nullptr;
		}

		// 2. Spawn the equipped actor if we don't have one. Nobody owns it: it replicates for every machine to hold it
		if (!CurrentActor && TargetClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			CurrentActor = GetWorld()->SpawnActor<AActor>(TargetClass, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), SpawnParams);
			Slot->Instance.SpawnedActor = CurrentActor;
		}

		// 3. Hold it, as every other machine will once the slot replicates
		HoldItemActor(*Slot);
	}

	UKzItemComponent* ItemComp = Slot->Instance.SpawnedActor ? Slot->Instance.SpawnedActor->FindComponentByClass<UKzItemComponent>() : nullptr;

	Slot->Instance.ActiveEquippedAction = EquipFrag->OnEquippedAction.Clone(this);
	Slot->Instance.ActiveEquippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
	Slot->Instance.ActiveEquippedAction.SetContextProperty(TEXT("Equipment"), this);
	Slot->Instance.ActiveEquippedAction.SetContextProperty(TEXT("Item"), ItemComp);
	Slot->Instance.ActiveEquippedAction.SetContextProperty(TEXT("ItemActor"), ItemToEquip.SpawnedActor);
	Slot->Instance.ActiveEquippedAction.Run(this);

	OnItemEquippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
	OnItemEquippedAction.SetContextProperty(TEXT("Equipment"), this);
	OnItemEquippedAction.SetContextProperty(TEXT("ItemActor"), ItemToEquip.SpawnedActor);
	OnItemEquippedAction.SetContextProperty(TEXT("SlotID"), Slot->SlotID);
	OnItemEquippedAction.Run(this);

	// Create the authority machine's local visual, tell the item's listeners and replicate the slot.
	RefreshVisualForSlot(*Slot);
	ShowItem(*Slot, Slot->Instance.ItemDef);
	EquipmentList.MarkItemDirty(*Slot);
	RefreshEquippedGrants();

	// Notify server listeners
	OnItemEquipped.Broadcast(TargetSlot, ItemToEquip);

	return true;
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

bool UKzEquipmentComponent::EquipItemFromWorld(UKzItemComponent* ItemComp, FKzItemInstance& OutUnequippedItem, UKzInteractableComponent* PickedUpBy)
{
	if (!GetOwner()->HasAuthority() || !ItemComp || !ItemComp->ItemInstance.ItemDef)
	{
		return false;
	}

	return EquipItem(ItemComp->ItemInstance, OutUnequippedItem, PickedUpBy);
}

bool UKzEquipmentComponent::UnequipItem(FGameplayTag SlotID, FKzItemInstance& OutUnequippedItem)
{
	const UKzEquipmentLayout* Layout = GetLayout();
	if (!GetOwner()->HasAuthority() || !Layout)
	{
		return false;
	}

	FEquippedSlot* Slot = EquipmentList.Slots.FindByKey(Layout->ResolveSlotID(SlotID));
	if (!Slot || !Slot->Instance.IsValid())
	{
		return false;
	}

	OutUnequippedItem = Slot->Instance;

	const UKzItemFragment_Equippable* EquipFrag = OutUnequippedItem.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>();
	const UKzItemFragment_Storable* StoreFrag = OutUnequippedItem.ItemDef->FindFragmentByClass<UKzItemFragment_Storable>();

	OutUnequippedItem.ActiveEquippedAction.Reset();
	OutUnequippedItem.ActiveEquippedAction = FScriptableAction();

	// Destroy the authority machine's local cosmetic mesh and put the item away from its listeners; clients do both via the callbacks.
	ClearVisualForSlot(*Slot);
	ShowItem(*Slot, nullptr);
	OutUnequippedItem.SpawnedComponent = nullptr;

	Slot->Instance = FKzItemInstance(); // Clear the slot
	Slot->PickedUpBy = nullptr;
	EquipmentList.MarkItemDirty(*Slot);

	// The slot lets go of the actor here, as every other machine will once the slot replicates
	HoldItemActor(*Slot);

	// Still held by other slots, it stays with them. Let go by its last, it goes to the backpack or drops, unless it is being destroyed
	AActor* ItemActor = OutUnequippedItem.SpawnedActor;
	const UKzItemComponent* ItemComp = ItemActor ? ItemActor->FindComponentByClass<UKzItemComponent>() : nullptr;
	if ((!ItemComp || !ItemComp->IsHeld()) && !(ItemActor && ItemActor->IsActorBeingDestroyed()))
	{
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

			if (AActor* DroppedActor = OutUnequippedItem.SpawnedActor)
			{
				// Safe drop: push the dropped item clear of the character so it doesn't
				// land inside the capsule. Custom-attachment items manage their own
				// placement, so we must not relocate them.
				if (!EquipFrag || !EquipFrag->bUseCustomAttachment)
				{
					AActor* OwnerActor = GetOwner();

					const float OwnerRadius = OwnerActor->GetSimpleCollisionRadius();

					FVector ItemOrigin, ItemExtent;
					DroppedActor->GetActorBounds(true, ItemOrigin, ItemExtent);

					const float SafeDistance = OwnerRadius + ItemExtent.GetMax() + 5.0f;

					FVector OwnerLoc = OwnerActor->GetActorLocation();
					FVector ForwardDir = OwnerActor->GetActorForwardVector();

					// Keep the current Z so it falls naturally from the hand socket, but push it away in the XY plane.
					FVector DropLocation = DroppedActor->GetActorLocation();
					DropLocation.X = OwnerLoc.X + (ForwardDir.X * SafeDistance);
					DropLocation.Y = OwnerLoc.Y + (ForwardDir.Y * SafeDistance);

					DroppedActor->SetActorLocation(DropLocation, false, nullptr, ETeleportType::TeleportPhysics);
				}

				// Enable physics only if the KzItemComponent dictates it, and throw it along with the character
				if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(DroppedActor->GetRootComponent()))
				{
					const UKzItemComponent* DroppedItem = DroppedActor->FindComponentByClass<UKzItemComponent>();
					if (!DroppedItem || DroppedItem->bSimulatePhysics)
					{
						RootPrim->SetSimulatePhysics(true);
						RootPrim->SetPhysicsLinearVelocity(GetOwner()->GetVelocity());
						RootPrim->AddTorqueInRadians(FVector::CrossProduct(FVector::UpVector, GetOwner()->GetVelocity()), NAME_None, true);
					}
				}
			}
		}
	}

	OnItemUnequippedAction.SetContextProperty(TEXT("Instigator"), GetOwner());
	OnItemUnequippedAction.SetContextProperty(TEXT("Equipment"), this);
	OnItemUnequippedAction.SetContextProperty(TEXT("ItemActor"), OutUnequippedItem.SpawnedActor);
	OnItemUnequippedAction.SetContextProperty(TEXT("SlotID"), Slot->SlotID);
	OnItemUnequippedAction.Run(this);

	RefreshEquippedGrants();
	OnItemUnequipped.Broadcast(SlotID, OutUnequippedItem);
	return true;
}

bool UKzEquipmentComponent::StowItem(FGameplayTag SlotID)
{
	const UKzEquipmentLayout* Layout = GetLayout();
	if (!GetOwner()->HasAuthority() || !Layout)
	{
		return false;
	}

	FEquippedSlot* From = EquipmentList.Slots.FindByKey(Layout->ResolveSlotID(SlotID));
	const UKzItemFragment_Equippable* Equippable = From && From->Instance.IsValid() ? From->Instance.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
	FEquippedSlot* To = Equippable && Equippable->StowSlot.IsValid() ? EquipmentList.Slots.FindByKey(Layout->ResolveSlotID(Equippable->StowSlot)) : nullptr;
	if (!To || To == From || To->Instance.IsValid())
	{
		return false;
	}

	MoveItem(*From, *To);
	return true;
}

bool UKzEquipmentComponent::DrawItem(FGameplayTag SlotID)
{
	const UKzEquipmentLayout* Layout = GetLayout();
	if (!GetOwner()->HasAuthority() || !Layout)
	{
		return false;
	}

	const FGameplayTag TargetSlot = Layout->ResolveSlotID(SlotID);
	FEquippedSlot* To = EquipmentList.Slots.FindByKey(TargetSlot);
	if (!To || To->Instance.IsValid())
	{
		return false;
	}

	FEquippedSlot* From = EquipmentList.Slots.FindByPredicate([this, Layout, TargetSlot](const FEquippedSlot& Slot)
	{
		const UKzItemFragment_Equippable* Equippable = Slot.Instance.IsValid() ? Slot.Instance.ItemDef->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
		return Equippable && Layout->ResolveSlotID(Equippable->TargetSlot) == TargetSlot && IsStowed(Slot, *Equippable);
	});
	if (!From)
	{
		return false;
	}

	MoveItem(*From, *To);
	return true;
}

bool UKzEquipmentComponent::IsStowed(const FEquippedSlot& Slot, const UKzItemFragment_Equippable& Equippable) const
{
	const UKzEquipmentLayout* Layout = GetLayout();
	return Layout && Equippable.StowSlot.IsValid() && Slot.SlotID == Layout->ResolveSlotID(Equippable.StowSlot) && Slot.SlotID != Layout->ResolveSlotID(Equippable.TargetSlot);
}

void UKzEquipmentComponent::MoveItem(FEquippedSlot& From, FEquippedSlot& To)
{
	ClearVisualForSlot(From);
	ShowItem(From, nullptr);

	To.Instance = From.Instance;
	To.PickedUpBy = From.PickedUpBy;
	From.Instance = FKzItemInstance();
	From.PickedUpBy = nullptr;

	// The item's actor goes along: the one slot lets go of it and the other holds it, at its socket, hidden there when a stowed mesh stands in for it
	HoldItemActor(From);
	HoldItemActor(To);

	RefreshVisualForSlot(To);
	ShowItem(To, To.Instance.ItemDef);
	EquipmentList.MarkItemDirty(From);
	EquipmentList.MarkItemDirty(To);
	RefreshEquippedGrants();

	OnItemUnequipped.Broadcast(From.SlotID, To.Instance);
	OnItemEquipped.Broadcast(To.SlotID, To.Instance);
}

const FKzItemInstance* UKzEquipmentComponent::FindItemInSlot(FGameplayTag SlotID) const
{
	const UKzEquipmentLayout* Layout = GetLayout();
	if (!Layout)
	{
		return nullptr;
	}

	FGameplayTag TargetSlot = Layout->ResolveSlotID(SlotID);

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