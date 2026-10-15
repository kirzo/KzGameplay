// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Equipment/KzEquipListener.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Items/KzItemComponent.h"
#include "Items/KzItemDefinition.h"
#include "GameFramework/Actor.h"

UKzItemFragment_Equippable::UKzItemFragment_Equippable()
{
	OnEquippedAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	OnEquippedAction.AddContextProperty<UKzEquipmentComponent*>(TEXT("Equipment"));
	OnEquippedAction.AddContextProperty<UKzItemComponent*>(TEXT("Item"));
	OnEquippedAction.AddContextProperty<AActor*>(TEXT("ItemActor"));
}

void UKzItemFragment_Equippable::NotifyEquipped(const FKzEquipEvent& Event) const
{
	if (const UKzItemDefinition* Item = GetTypedOuter<UKzItemDefinition>())
	{
		for (const UKzItemFragment* Fragment : Item->Fragments)
		{
			if (Fragment && Fragment->Implements<UKzEquipListener>())
			{
				IKzEquipListener::Execute_OnEquipped(Fragment, Event);
			}
		}
	}
}

void UKzItemFragment_Equippable::NotifyUnequipped(const FKzEquipEvent& Event) const
{
	if (const UKzItemDefinition* Item = GetTypedOuter<UKzItemDefinition>())
	{
		for (const UKzItemFragment* Fragment : Item->Fragments)
		{
			if (Fragment && Fragment->Implements<UKzEquipListener>())
			{
				IKzEquipListener::Execute_OnUnequipped(Fragment, Event);
			}
		}
	}
}