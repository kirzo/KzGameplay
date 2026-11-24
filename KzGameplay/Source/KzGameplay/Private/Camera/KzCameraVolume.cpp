// Copyright 2026 kirzo

#include "Camera/KzCameraVolume.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AKzCameraVolume::AKzCameraVolume()
{
	// Camera changes add up along the way, as markers' do.
	bKeepPastExit = true;

#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		static ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture(TEXT("/KzGameplay/EditorResources/CameraVolume"));
		SpriteComponent->Sprite = SpriteTexture.Get();
	}
#endif
}
