// Copyright 2026 kirzo

#include "Inventory/ScriptableTask_GrantItem.h"
#include "Inventory/KzInventoryComponent.h"
#include "Items/KzItemDefinition.h"
#include "Items/KzItemInstance.h"

void UScriptableTask_GrantItem::BeginTask()
{
	if (IsValid(TargetActor) && !ItemToGrant.IsNull())
	{
		if (UKzInventoryComponent* InventoryComp = TargetActor->FindComponentByClass<UKzInventoryComponent>())
		{
			if (const UKzItemDefinition* LoadedItemDef = ItemToGrant.LoadSynchronous())
			{
				// Create the instance and add it
				FKzItemInstance NewInstance;
				NewInstance.ItemDef = LoadedItemDef;
				NewInstance.Quantity = Quantity;

				InventoryComp->TryAddItem(LoadedItemDef, Quantity);
			}
		}
	}

	// Always notify the framework that this task has finished its execution
	Finish();
}

void UScriptableTask_GrantItem::ResetTask()
{
	if (bRevertOnReset && IsValid(TargetActor) && !ItemToGrant.IsNull())
	{
		if (UKzInventoryComponent* InventoryComp = TargetActor->FindComponentByClass<UKzInventoryComponent>())
		{
			if (const UKzItemDefinition* LoadedItemDef = ItemToGrant.LoadSynchronous())
			{
				InventoryComp->RemoveItem(LoadedItemDef, Quantity);
			}
		}
	}
}

#if WITH_EDITOR
FText UScriptableTask_GrantItem::GetDisplayTitle() const
{
	FString TargetName;
	// Use the framework's binding system to get the display name, fallback to ActorLabel
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_GrantItem, TargetActor), TargetName))
	{
		TargetName = TargetActor ? TargetActor->GetActorLabel() : TEXT("None");
	}

	if (!ItemToGrant.IsNull())
	{
		return FText::Format(INVTEXT("Grant Item [{0}] (x{1}) to {2}"), FText::FromString(ItemToGrant.GetAssetName()), Quantity, FText::FromString(TargetName));
	}

	return INVTEXT("Grant Item");
}
#endif