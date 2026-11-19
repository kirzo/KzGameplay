// Copyright 2026 kirzo

#pragma once

#include "KzLibMinimal.h"
#include "Scoring/KzTargetScorer.h"
#include "KzTargetScorer_Facing.generated.h"

/**
 * Scores a target by how squarely it faces the origin: the angle between the target's forward and the way from it to the origin.
 * It is the Angle Scorer seen from the other end. That one tells whether the origin looks at the target, and turning on the spot
 * changes it; this one tells whether the target looks back, and only moving round it does.
 * An interactable that does not limit its approach has no front, and scores 1 from any side.
 */
UCLASS(DisplayName = "Facing Scorer")
class KZGAMEPLAY_API UKzTargetScorer_Facing : public UKzTargetScorer
{
	GENERATED_BODY()

public:
	/**
	 * The angle off the target's forward, in degrees, at which the score reaches 0.0.
	 * Seen head-on (0 degrees), the score is 1.0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facing Settings", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float MaxAngle = 90.0f;

protected:
	virtual float CalculateScore(const FKzTransformSource& Origin, const FKzTransformSource& Target) const override;
};