// Copyright 2026 kirzo

#include "KzGameplayEditorStyle.h"

FKzGameplayEditorStyle::FKzGameplayEditorStyle()
	: TKzEditorStyle_Base(FName("KzGameplayEditorStyle"))
{
	SetupPluginResources(TEXT("KzGameplay"));

	// White line icon for lists, tabs and menus; colour figure for the Content Browser
	for (const TCHAR* Asset : { TEXT("InputProfile"), TEXT("ItemDefinition"), TEXT("EquipmentLayout"), TEXT("CapabilitySet"), TEXT("SteeringProfile") })
	{
		Set(*FString::Printf(TEXT("ClassIcon.Kz%s"), Asset), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Asset, TEXT(".svg")), FVector2D(16.0f, 16.0f)));
		Set(*FString::Printf(TEXT("ClassThumbnail.Kz%s"), Asset), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Asset + TEXT("Color"), TEXT(".svg")), FVector2D(64.0f, 64.0f)));
	}

	// Components only show the small icon, in the component tree and the Add Component menu
	for (const TCHAR* Component : { TEXT("Interactable"), TEXT("Interactor"), TEXT("InteractableGroup"), TEXT("Sensor"), TEXT("Sensable"), TEXT("Inventory"), TEXT("Save"), TEXT("AnimLayer") })
	{
		Set(*FString::Printf(TEXT("ClassIcon.Kz%sComponent"), Component), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Component, TEXT(".svg")), FVector2D(16.0f, 16.0f)));
	}

	// A component that holds or applies one of the assets above wears that asset's icon
	const TCHAR* const AssetComponents[][2] = {
		{ TEXT("Capability"), TEXT("CapabilitySet") },
		{ TEXT("InputHandler"), TEXT("InputProfile") },
		{ TEXT("Equipment"), TEXT("EquipmentLayout") },
		{ TEXT("Item"), TEXT("ItemDefinition") },
		{ TEXT("Steering"), TEXT("SteeringProfile") },
	};
	for (const auto& Pair : AssetComponents)
	{
		Set(*FString::Printf(TEXT("ClassIcon.Kz%sComponent"), Pair[0]), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Pair[1], TEXT(".svg")), FVector2D(16.0f, 16.0f)));
	}
}