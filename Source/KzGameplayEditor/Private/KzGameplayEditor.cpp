// Copyright 2026 kirzo

#pragma once

#include "KzGameplayEditor.h"
#include "KzGameplayEditorStyle.h"

#include "Items/KzItemDefinition.h"
#include "Equipment/KzEquipmentLayout.h"
#include "Input/KzInputProfile.h"

#define LOCTEXT_NAMESPACE "FKzGameplayEditorModule"

void FKzGameplayEditorModule::OnStartupModule()
{
	FKzGameplayEditorStyle::Initialize();

	RegisterAssetTypeAction<UKzItemDefinition>(KzAssetCategoryBit, INVTEXT("Item"), FColor::FromHex("#F4A261"), { INVTEXT("Gameplay") });
	RegisterAssetTypeAction<UKzEquipmentLayout>(KzAssetCategoryBit, INVTEXT("Equipment Layout"), FColor::FromHex("#2A9D8F"), { INVTEXT("Gameplay") });
	RegisterAssetTypeAction<UKzInputProfile>(KzAssetCategoryBit, INVTEXT("Input Profile"), FColor::FromHex("#00CBA9"), { INVTEXT("Input") });
}

void FKzGameplayEditorModule::OnShutdownModule()
{
	FKzGameplayEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzGameplayEditorModule, KzGameplayEditor);