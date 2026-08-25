// Copyright 2026 kirzo

#pragma once

#include "KzGameplayEditor.h"
#include "KzGameplayEditorStyle.h"

#include "Items/KzItemDefinition.h"
#include "Equipment/KzEquipmentLayout.h"
#include "Input/KzInputProfile.h"
#include "Capabilities/KzCapabilitySet.h"

#include "Editors/KzArrayAssetEditor.h"
#include "Widgets/SKzPropertyStack.h"
#include "UObject/StructOnScope.h"
#include "GameplayTagContainer.h"

#define LOCTEXT_NAMESPACE "FKzGameplayEditorModule"

/** Builds read-only rows for an equipment layout's inherited slots, walking the parent chain. */
static TArray<TSharedPtr<FKzStackRow>> BuildEquipmentLayoutInheritedRows(UObject* Asset)
{
	TArray<TSharedPtr<FKzStackRow>> Rows;
	const UKzEquipmentLayout* Layout = Cast<UKzEquipmentLayout>(Asset);
	if (!Layout) { return Rows; }

	// SlotIDs already defined by this layout (or a nearer ancestor as we descend) are overrides.
	TSet<FGameplayTag> DefinedIds;
	for (const FKzEquipmentSlotDefinition& Slot : Layout->Slots)
	{
		DefinedIds.Add(Slot.SlotID);
	}

	const UScriptStruct* SlotStruct = FKzEquipmentSlotDefinition::StaticStruct();

	// Walk ancestors, nearest first, guarding against a broken runtime cycle.
	TSet<const UKzEquipmentLayout*> Visited;
	const UKzEquipmentLayout* Ancestor = Layout->ParentLayout;
	while (Ancestor && !Visited.Contains(Ancestor))
	{
		Visited.Add(Ancestor);

		const FText Group = FText::Format(LOCTEXT("InheritedFrom", "Inherited from {0}"), FText::FromString(Ancestor->GetName()));
		const FText SourceTag = FText::FromString(Ancestor->GetName());

		for (const FKzEquipmentSlotDefinition& Slot : Ancestor->Slots)
		{
			TSharedPtr<FKzStackRow> Row = MakeShared<FKzStackRow>();
			Row->bEditable = false;
			Row->GroupName = Group;
			Row->SourceTag = SourceTag;
			Row->bIsOverridden = DefinedIds.Contains(Slot.SlotID);
			Row->DisplayLabel = Slot.DisplayName.IsEmpty() ? FText::FromString(Slot.SlotID.ToString()) : Slot.DisplayName;

			TSharedPtr<FStructOnScope> Snapshot = MakeShared<FStructOnScope>(SlotStruct);
			SlotStruct->CopyScriptStruct(Snapshot->GetStructMemory(), &Slot);
			Row->Snapshot = Snapshot;

			Rows.Add(Row);

			// Deeper ancestors defining the same slot are overridden by this one.
			DefinedIds.Add(Slot.SlotID);
		}

		Ancestor = Ancestor->ParentLayout;
	}

	return Rows;
}

void FKzGameplayEditorModule::OnStartupModule()
{
	FKzGameplayEditorStyle::Initialize();

	TArray<FKzArrayEditorTabConfig> ItemTabs;
	ItemTabs.Add(FKzArrayEditorTabConfig(
		GET_MEMBER_NAME_CHECKED(UKzItemDefinition, Fragments),
		INVTEXT("Fragment")));

	TArray<FKzArrayEditorTabConfig> InputTabs;
	InputTabs.Add(FKzArrayEditorTabConfig(
		GET_MEMBER_NAME_CHECKED(UKzInputProfile, InputActions),
		INVTEXT("Action")));

	TArray<FKzArrayEditorTabConfig> LayoutTabs;
	{
		FKzArrayEditorTabConfig SlotsTab(GET_MEMBER_NAME_CHECKED(UKzEquipmentLayout, Slots), INVTEXT("Slot"));
		SlotsTab.ImmutableRowsSource = &BuildEquipmentLayoutInheritedRows;
		LayoutTabs.Add(SlotsTab);
	}

	RegisterAssetTypeAction<UKzItemDefinition, FKzArrayAssetEditor>(KzAssetCategoryBit, INVTEXT("Item"), FColor::FromHex("#F4A261"), { INVTEXT("Gameplay") }, ItemTabs);
	RegisterAssetTypeAction<UKzEquipmentLayout, FKzArrayAssetEditor>(KzAssetCategoryBit, INVTEXT("Equipment Layout"), FColor::FromHex("#2A9D8F"), { INVTEXT("Gameplay") }, LayoutTabs);
	RegisterAssetTypeAction<UKzInputProfile, FKzArrayAssetEditor>(KzAssetCategoryBit, INVTEXT("Input Profile"), FColor::FromHex("#00CBA9"), { INVTEXT("Input") }, InputTabs);

	// Keyed by tag rather than an array, so the plain details editor fits it better than the array editor
	RegisterAssetTypeAction<UKzCapabilitySet>(KzAssetCategoryBit, INVTEXT("Capability Set"), FColor::FromHex("#E76F51"), { INVTEXT("Gameplay") });
}

void FKzGameplayEditorModule::OnShutdownModule()
{
	FKzGameplayEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzGameplayEditorModule, KzGameplayEditor);