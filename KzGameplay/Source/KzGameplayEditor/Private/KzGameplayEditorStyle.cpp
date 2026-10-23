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

	// Scriptable tasks and conditions wear the icon of what they act on, badged where two share it; ScriptableFramework tints them
	const TCHAR* const ScriptableObjects[][2] = {
		{ TEXT("ScriptableTask_GrantCapability"), TEXT("ScriptableTask_GrantCapability") },
		{ TEXT("ScriptableCondition_HasCapability"), TEXT("ScriptableCondition_HasCapability") },
		{ TEXT("ScriptableTask_EquipItem"), TEXT("EquipmentLayout") },
		{ TEXT("ScriptableTask_GrantItem"), TEXT("ItemDefinition") },
		{ TEXT("ScriptableTask_LinkAnimLayers"), TEXT("AnimLayer") },
		{ TEXT("ScriptableTask_LinkAnimLayerByQuery"), TEXT("AnimLayer") },
	};
	for (const auto& Pair : ScriptableObjects)
	{
		Set(*FString::Printf(TEXT("ClassIcon.%s"), Pair[0]), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Pair[1], TEXT(".svg")), FVector2D(16.0f, 16.0f)));
	}

	// Item fragments: a broken gem for the base, their own drawing, or the icon of the asset or component they feed
	const TCHAR* const Fragments[][2] = {
		{ TEXT("KzItemFragment"), TEXT("ItemFragment") },
		{ TEXT("KzItemFragment_MeleeWeapon"), TEXT("MeleeWeapon") },
		{ TEXT("KzItemFragment_InitialStats"), TEXT("InitialStats") },
		{ TEXT("KzItemFragment_Equippable"), TEXT("EquipmentLayout") },
		{ TEXT("KzItemFragment_Storable"), TEXT("Inventory") },
	};
	for (const auto& Pair : Fragments)
	{
		Set(*FString::Printf(TEXT("ClassIcon.%s"), Pair[0]), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Pair[1], TEXT(".svg")), FVector2D(16.0f, 16.0f)));
	}

	// The Database fragment wears KzLib's database
	if (const TSharedPtr<IPlugin> KzLibPlugin = IPluginManager::Get().FindEnabledPlugin(TEXT("KzLib")))
	{
		Set("ClassIcon.KzItemFragment_Database", new FSlateVectorImageBrush(KzLibPlugin->GetBaseDir() / TEXT("Resources/Icons/Database.svg"), FVector2D(16.0f, 16.0f)));
	}

	// KzGameplayLimbic's pose fragments, direct and by query, wear Limbic's puppeteer through their base
	if (const TSharedPtr<IPlugin> Limbic = IPluginManager::Get().FindEnabledPlugin(TEXT("Limbic")))
	{
		Set("ClassIcon.KzItemFragment_LimbicPoseBase", new FSlateVectorImageBrush(Limbic->GetBaseDir() / TEXT("Resources/Icons/Puppeteer.svg"), FVector2D(16.0f, 16.0f)));
	}
}