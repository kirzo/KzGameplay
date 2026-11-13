// Copyright 2026 kirzo

#include "Factories/ItemDefinitionActorFactory.h"
#include "Items/KzItemDefinition.h"

UItemDefinitionActorFactory::UItemDefinitionActorFactory()
{
	DisplayName = INVTEXT("Item");
}

bool UItemDefinitionActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (GetDefaultActorClass(AssetData))
	{
		return true;
	}

	OutErrorMsg = INVTEXT("Only item definitions with a World Actor Class can be placed.");
	return false;
}

UClass* UItemDefinitionActorFactory::GetDefaultActorClass(const FAssetData& AssetData)
{
	// Check the class first so other assets are never loaded.
	const UKzItemDefinition* Item = AssetData.IsInstanceOf<UKzItemDefinition>() ? Cast<UKzItemDefinition>(AssetData.GetAsset()) : nullptr;
	return Item ? Item->WorldActorClass.LoadSynchronous() : nullptr;
}