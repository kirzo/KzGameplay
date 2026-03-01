// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Scoring/KzTargetScorer.h"
#include "KzTargetScorer_Distance.generated.h"

/**
 * Scores a target based on its distance to the origin.
 * Closer targets receive a higher score (approaching 1.0).
 */
UCLASS(DisplayName = "Distance Scorer")
class KZGAMEPLAY_API UKzTargetScorer_Distance : public UKzTargetScorer
{
	GENERATED_BODY()

public:
	/** The maximum expected distance. Targets beyond this will score 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Settings", meta = (ClampMin = 1, Units = Centimeters))
	float MaxDistance = 1000.0f;

	/** If true, distance is calculated only on the XY plane, ignoring Z (height) differences. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Settings")
	bool bUseHorizontalDistanceOnly = false;

protected:
	virtual float CalculateScore_Implementation(const AActor* Origin, const AActor* Target) const override;
};