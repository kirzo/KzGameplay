// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_LimbicPose.h"
#include "Components/KzDatabaseComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "LimbicPuppeteer.h"
#include "LimbicSlotLibrary.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Limbic_Slot_Hold, "Limbic.Slot.Hold");

namespace
{
	// The skeletal mesh the item hangs from: the character's, for a hand slot.
	USkeletalMeshComponent* GetPosedMesh(const FKzEquipEvent& Event)
	{
		return Event.Equipment ? Cast<USkeletalMeshComponent>(Event.Equipment->GetAttachParentForSlot(Event.SlotID)) : nullptr;
	}
}

UKzItemFragment_LimbicPose::UKzItemFragment_LimbicPose()
{
	Slot = TAG_Limbic_Slot_Hold;
}

void UKzItemFragment_LimbicPose::GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const
{
	OutRequired.Add(UKzItemFragment_Equippable::StaticClass());
}

void UKzItemFragment_LimbicPose::OnEquipped_Implementation(const FKzEquipEvent& Event) const
{
	USkeletalMeshComponent* Mesh = GetPosedMesh(Event);
	const AActor* Holder = Event.Equipment ? Event.Equipment->GetOwner() : nullptr;
	ULimbicPuppeteer* Pose = UKzDatabaseComponent::ResolveFor(Holder, PuppeteerQuery, Puppeteer).LoadSynchronous();
	if (!Pose)
	{
		Pose = Puppeteer.LoadSynchronous();
	}
	if (Mesh && Pose)
	{
		ULimbicSlotLibrary::SetLimbicSlot(Mesh, Slot, Pose, BlendIn);
	}
}

void UKzItemFragment_LimbicPose::OnUnequipped_Implementation(const FKzEquipEvent& Event) const
{
	if (USkeletalMeshComponent* Mesh = GetPosedMesh(Event))
	{
		ULimbicSlotLibrary::StopLimbicSlot(Mesh, Slot, BlendOut);
	}
}