// Copyright 2026 kirzo

#include "KzLightingVolume.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AKzLightingVolume::AKzLightingVolume()
{
#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzGameplay/EditorResources/LightingVolume"));
		SpriteComponent->Sprite = SpriteTexture.Get();
	}
#endif
}