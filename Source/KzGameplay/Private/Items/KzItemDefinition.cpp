// Copyright 2026 kirzo

#include "Items/KzItemDefinition.h"
#include "Items/KzItemComponent.h"
#include "Inventory/KzInventoryComponent.h"
#include "Equipment/KzEquipmentComponent.h"

#include "GameFramework/Actor.h"

UKzItemDefinition::UKzItemDefinition()
{
	OnAcquiredAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	OnAcquiredAction.AddContextProperty<UKzInventoryComponent*>(TEXT("Inventory"));

	OnEquippedAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	OnEquippedAction.AddContextProperty<UKzEquipmentComponent*>(TEXT("Equipment"));
	OnEquippedAction.AddContextProperty<UKzItemComponent*>(TEXT("Item"));
	OnEquippedAction.AddContextProperty<AActor*>(TEXT("ItemActor"));
}