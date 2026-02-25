// Copyright 2026 kirzo

#pragma once

#include "KzGameplayEditor.h"

#include "Items/ItemDefinition.h"
#include "Equipment/EquipmentLayout.h"

#define LOCTEXT_NAMESPACE "FKzGameplayEditorModule"

void FKzGameplayEditorModule::OnStartupModule()
{
	const TArray<FText> SubMenus = { INVTEXT("Gameplay") };
	RegisterAssetTypeAction<UItemDefinition>(KzAssetCategoryBit, INVTEXT("Item"), FColor::FromHex("#F4A261"), SubMenus);
	RegisterAssetTypeAction<UEquipmentLayout>(KzAssetCategoryBit, INVTEXT("Equipment Layout"), FColor::FromHex("#2A9D8F"), SubMenus);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzGameplayEditorModule, KzGameplayEditor);