// Copyright 2026 kirzo

#include "Scoring/Scorers/KzTargetScorer_Angle.h"
#include "GameFramework/Actor.h"

float UKzTargetScorer_Angle::CalculateScore_Implementation(const FKzTransformSource& Origin, const FKzTransformSource& Target) const
{
	if (!Origin.IsValid() || !Target.IsValid()) return 0.0f;

	const FTransform OriginTransform = Origin.GetTransform();

	// Use the Origin's forward vector to determine where it's looking.
	const FVector OriginForward = OriginTransform.GetRotation().GetForwardVector();

	// Get the direction pointing from the Origin to the Target
	const FVector DirectionToTarget = Target.GetLocation() - OriginTransform.GetLocation();

	float AngleDifference = 0.0f;

	// Calculate the angle in degrees using KzLib
	if (ScoreMode == EKzPlane::Horizontal)
	{
		AngleDifference = FKzMath::GetHorizontalAngle(OriginForward, DirectionToTarget);
	}
	else
	{
		AngleDifference = FKzMath::GetVerticalAngleDifference(OriginForward, DirectionToTarget);
	}

	// Normalize the score (0.0 to 1.0)
	// 0 degrees difference = 1.0 score
	// MaxAngle (or greater) = 0.0 score
	const float NormalizedScore = 1.0f - FMath::Clamp(AngleDifference / MaxAngle, 0.0f, 1.0f);

	return NormalizedScore;
}