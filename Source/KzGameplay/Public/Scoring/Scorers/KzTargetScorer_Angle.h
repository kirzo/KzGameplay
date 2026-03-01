// Copyright 2026 kirzo

#pragma once

#include "KzLibMinimal.h"
#include "Scoring/KzTargetScorer.h"
#include "KzTargetScorer_Angle.generated.h"

/**
 * Scores a target based on the angle difference between the Origin's forward vector
 * and the direction to the target. Uses KzMath for optimized pure C++ calculations.
 */
UCLASS(DisplayName = "Angle Scorer")
class KZGAMEPLAY_API UKzTargetScorer_Angle : public UKzTargetScorer
{
	GENERATED_BODY()

public:
	/** Determines if we are checking the horizontal (yaw) or vertical (pitch) angle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle Settings")
	EKzPlane ScoreMode = EKzPlane::Horizontal;

	/**
	 * The maximum angle difference in degrees.
	 * If the target is exactly at this angle or further, the score will be 0.0.
	 * If it is exactly in front (0 degrees), the score will be 1.0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle Settings", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float MaxAngle = 90.0f;

protected:
	virtual float CalculateScore_Implementation(const AActor* Origin, const AActor* Target) const override;
};