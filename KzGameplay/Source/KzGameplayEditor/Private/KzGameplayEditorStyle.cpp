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
}