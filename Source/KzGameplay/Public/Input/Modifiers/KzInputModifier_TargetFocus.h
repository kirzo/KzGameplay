// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Input/KzInputModifier.h"
#include "Misc/KzTransformSource.h"
#include "KzInputModifier_TargetFocus.generated.h"

/**
 * Input modifier that forces the input vector to point towards a specific target.
 * Allows a configurable angular threshold (cone). If the input exceeds this angle,
 * it is clamped to the edge of the allowed cone.
 */
UCLASS(meta = (DisplayName = "Target Focus Constraint"))
class KZGAMEPLAY_API UKzInputModifier_TargetFocus : public UKzInputModifier
{
	GENERATED_BODY()

public:
	/** The target to focus the input towards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus", meta = (ExposeOnSpawn = "true"))
	FKzTransformSource TargetSource;

	/**
	 * The allowed angular deviation from the direct line to the target (in degrees).
	 * 0 = Strict mathematical lock. 90 = Allows any forward-facing input.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus", meta = (ExposeOnSpawn = "true", ClampMin = 0.0, ClampMax = 180.0))
	float ConeHalfAngle = 15.0f;

	/**
	 * If true, and the player provides 0 input, the modifier forces the input to point
	 * directly at the target. Ideal for auto-aim or forced look-at mechanics.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus")
	bool bForceFocusOnZeroInput = true;

protected:
	virtual FVector ModifyInput_Implementation(const AActor* Avatar, const FVector& OriginalInput, const FVector& CurrentInput) const override;
};