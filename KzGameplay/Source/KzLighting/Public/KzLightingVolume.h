// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Actors/KzAlphaVolume.h"
#include "KzLightingProfile.h"
#include "KzLightingVolume.generated.h"

/**
 * Adds a lighting layer to the rig while the viewpoint is in it, weighted by the volume's alpha.
 * A progressive volume blends in space and keeps its layer past the exit face. A non-progressive one blends in time and removes it on leaving, unless it keeps past exit.
 * The layer goes away if the volume unloads, so an unbound volume in a streamed level brings that level's look while it is loaded.
 */
UCLASS(Blueprintable)
class KZLIGHTING_API AKzLightingVolume : public AKzAlphaVolume
{
	GENERATED_BODY()

public:
	AKzLightingVolume();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting")
	FKzLightingLayerSettings Layer;

	/** Covers the whole world, whatever its shape, and blends in time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting")
	bool bUnbound = false;
};