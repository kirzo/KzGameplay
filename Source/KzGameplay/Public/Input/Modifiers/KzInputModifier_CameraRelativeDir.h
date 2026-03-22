// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "KzInputModifier_CameraRelativeDir.generated.h"

/**
 * Input modifier that translates raw 2D input into a normalized 3D World Direction
 * rotated by the current camera's Yaw.
 * Ideal for Twin-Stick shooters or absolute directional aiming.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Camera Relative Direction"))
class KZGAMEPLAY_API UKzInputModifier_CameraRelativeDir : public UKzInputModifier
{
	GENERATED_BODY()

public:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const override;
};