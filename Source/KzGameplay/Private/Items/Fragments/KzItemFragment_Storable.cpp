// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_Storable.h"
#include "Inventory/KzInventoryComponent.h"
#include "GameFramework/Actor.h"

UKzItemFragment_Storable::UKzItemFragment_Storable()
{
	OnAcquiredAction.AddContextProperty<AActor*>(TEXT("Instigator"));
	OnAcquiredAction.AddContextProperty<UKzInventoryComponent*>(TEXT("Inventory"));
}