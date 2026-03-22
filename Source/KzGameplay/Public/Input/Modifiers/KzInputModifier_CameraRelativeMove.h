// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "KzInputModifier_CameraRelativeMove.generated.h"

/**
 * Input modifier that translates raw 2D input (X, Y) into a 3D World Space vector
 * relative to the current camera/view target's facing direction.
 * NOTE: To function correctly, this modifier should ideally be placed at the very
 * bottom of the modifier stack (highest priority / Index 0) so that subsequent
 * spatial modifiers (like Tethers or recoil) receive a valid World Space vector.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Camera Relative Movement"))
class KZGAMEPLAY_API UKzInputModifier_CameraRelativeMove : public UKzInputModifier
{
	GENERATED_BODY()

	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const override;
};