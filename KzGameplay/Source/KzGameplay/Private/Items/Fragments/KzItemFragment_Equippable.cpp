// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Items/KzItemComponent.h"
#include "GameFramework/Actor.h"

UKzItemFragment_Equippable::UKzItemFragment_Equippable()
{
	OnEquippedAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	OnEquippedAction.AddContextProperty<UKzEquipmentComponent*>(TEXT("Equipment"));
	OnEquippedAction.AddContextProperty<UKzItemComponent*>(TEXT("Item"));
	OnEquippedAction.AddContextProperty<AActor*>(TEXT("ItemActor"));
}