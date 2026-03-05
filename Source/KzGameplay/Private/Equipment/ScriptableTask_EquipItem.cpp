// Copyright 2026 kirzo

#include "Equipment/ScriptableTask_EquipItem.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "GameFramework/Actor.h"

void UScriptableTask_EquipItem::BeginTask()
{
	if (IsValid(TargetActor) && !ItemToEquip.IsNull())
	{
		if (UKzEquipmentComponent* EquipComp = TargetActor->FindComponentByClass<UKzEquipmentComponent>())
		{
			if (const UKzItemDefinition* LoadedItemDef = ItemToEquip.LoadSynchronous())
			{
				FKzItemInstance DroppedItem;
				EquipComp->EquipItemByDefinition(LoadedItemDef, DroppedItem);
			}
		}
	}

	// Always notify the framework that this task has finished its execution
	Finish();
}

void UScriptableTask_EquipItem::ResetTask()
{
	if (bRevertOnReset && IsValid(TargetActor) && !ItemToEquip.IsNull())
	{
		if (UKzEquipmentComponent* EquipComp = TargetActor->FindComponentByClass<UKzEquipmentComponent>())
		{
			if (const UKzItemDefinition* LoadedItemDef = ItemToEquip.LoadSynchronous())
			{
				if (const UKzItemFragment_Equippable* EquipFrag = LoadedItemDef->FindFragmentByClass<UKzItemFragment_Equippable>())
				{
					// To revert an equipment task, we simply unequip whatever is in the target slot
					FKzItemInstance UnequippedItem;
					EquipComp->UnequipItem(EquipFrag->TargetSlot, UnequippedItem);
				}
			}
		}
	}
}

#if WITH_EDITOR
FText UScriptableTask_EquipItem::GetDisplayTitle() const
{
	FString TargetName;
	// Use the framework's binding system to get the display name, fallback to ActorLabel
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_EquipItem, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	if (!ItemToEquip.IsNull())
	{
		return FText::Format(INVTEXT("Equip [{0}] on {1}"), FText::FromString(ItemToEquip.GetAssetName()), FText::FromString(TargetName));
	}

	return INVTEXT("Equip Item");
}
#endif