// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Actors/KzAlphaVolume.h"
#include "Camera/KzCameraTypes.h"
#include "KzCameraVolume.generated.h"

/**
 * Adds a layer to the camera rigs whose target is in it, weighted by the volume's alpha.
 * A progressive volume blends in space and keeps its layer past the exit face. A non-progressive one blends in time and removes it on leaving, unless it keeps past exit.
 * The layer goes away if the volume unloads, so keep it in the level of the camera's spline.
 */
UCLASS(Blueprintable)
class KZGAMEPLAY_API AKzCameraVolume : public AKzAlphaVolume
{
	GENERATED_BODY()

public:
	AKzCameraVolume();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FKzCameraLayerSettings Layer;
};
