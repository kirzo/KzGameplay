// Copyright 2026 kirzo

#include "Equipment/ScriptableTask_StowItem.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Equipment/KzEquipmentTags.h"
#include "GameFramework/Actor.h"

UScriptableTask_StowItem::UScriptableTask_StowItem()
{
	Slot = KzTags::Equipment::Slot_Hand_Main;
}

void UScriptableTask_StowItem::BeginTask()
{
	Apply(bDraw);
	Finish();
}

void UScriptableTask_StowItem::ResetTask()
{
	if (bRevertOnReset)
	{
		Apply(!bDraw);
	}
}

void UScriptableTask_StowItem::Apply(bool bDrawItem) const
{
	UKzEquipmentComponent* Equipment = IsValid(TargetActor) ? TargetActor->FindComponentByClass<UKzEquipmentComponent>() : nullptr;
	if (!Equipment)
	{
		return;
	}

	if (bDrawItem)
	{
		Equipment->DrawItem(Slot);
	}
	else
	{
		Equipment->StowItem(Slot);
	}
}

#if WITH_EDITOR
FText UScriptableTask_StowItem::GetDisplayTitle() const
{
	FString TargetName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_StowItem, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	return FText::Format(INVTEXT("{0} [{1}] on {2}"), bDraw ? INVTEXT("Draw") : INVTEXT("Stow"), FText::FromString(Slot.ToString()), FText::FromString(TargetName));
}
#endif
