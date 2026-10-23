// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_LimbicPose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Items/Fragments/KzItemFragment_Database.h"
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

UKzItemFragment_LimbicPoseBase::UKzItemFragment_LimbicPoseBase()
{
	Slot = TAG_Limbic_Slot_Hold;
}

void UKzItemFragment_LimbicPoseBase::GetRequiredFragments(TArray<TSubclassOf<UKzItemFragment>>& OutRequired) const
{
	OutRequired.Add(UKzItemFragment_Equippable::StaticClass());
}

void UKzItemFragment_LimbicPoseBase::OnEquipped_Implementation(const FKzEquipEvent& Event) const
{
	USkeletalMeshComponent* Mesh = GetPosedMesh(Event);
	ULimbicPuppeteer* Pose = Mesh ? GetPuppeteer(Event) : nullptr;
	if (Pose)
	{
		ULimbicSlotLibrary::SetLimbicSlot(Mesh, Slot, Pose, BlendIn);
	}
}

void UKzItemFragment_LimbicPoseBase::OnUnequipped_Implementation(const FKzEquipEvent& Event) const
{
	if (USkeletalMeshComponent* Mesh = GetPosedMesh(Event))
	{
		ULimbicSlotLibrary::StopLimbicSlot(Mesh, Slot, BlendOut);
	}
}

ULimbicPuppeteer* UKzItemFragment_LimbicPose::GetPuppeteer(const FKzEquipEvent& Event) const
{
	return Puppeteer.LoadSynchronous();
}

ULimbicPuppeteer* UKzItemFragment_LimbicPoseByQuery::GetPuppeteer(const FKzEquipEvent& Event) const
{
	const AActor* Holder = Event.Equipment ? Event.Equipment->GetOwner() : nullptr;
	return UKzItemFragment_Database::ResolveFor(Event.Item.Get(), Holder, Query, TSoftObjectPtr<ULimbicPuppeteer>()).LoadSynchronous();
}