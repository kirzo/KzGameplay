// Copyright 2026 kirzo

#include "Equipment/KzAnimNotify_StowItem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Equipment/KzEquipmentTags.h"

UKzAnimNotify_StowItem::UKzAnimNotify_StowItem()
{
	Slot = KzTags::Equipment::Slot_Hand_Main;
}

void UKzAnimNotify_StowItem::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// Only in game: an editor previewing the animation has no equipment to change
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !Owner->HasAuthority() || !Owner->GetWorld() || !Owner->GetWorld()->IsGameWorld())
	{
		return;
	}

	if (UKzEquipmentComponent* Equipment = Owner->FindComponentByClass<UKzEquipmentComponent>())
	{
		if (bDraw)
		{
			Equipment->DrawItem(Slot);
		}
		else
		{
			Equipment->StowItem(Slot);
		}
	}
}

FString UKzAnimNotify_StowItem::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("%s %s"), bDraw ? TEXT("Draw") : TEXT("Stow"), *Slot.ToString());
}
