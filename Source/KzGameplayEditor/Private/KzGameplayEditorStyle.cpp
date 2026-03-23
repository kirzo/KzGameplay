// Copyright 2026 kirzo

#include "KzGameplayEditorStyle.h"

FKzGameplayEditorStyle::FKzGameplayEditorStyle()
	: TKzEditorStyle_Base(FName("KzGameplayEditorStyle"))
{
	SetupPluginResources(TEXT("KzGameplay"));

	AddClassIcon(TEXT("KzInputProfile"), TEXT("KzInputProfile_16x"));
	AddClassThumbnail(TEXT("KzInputProfile"), TEXT("KzInputProfile_64x"));
}