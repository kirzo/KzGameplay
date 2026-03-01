// Copyright 2026 kirzo

#include "Scoring/Scorers/KzTargetScorer_Distance.h"
#include "GameFramework/Actor.h"

float UKzTargetScorer_Distance::CalculateScore_Implementation(const AActor* Origin, const AActor* Target) const
{
	if (!Origin || !Target) return 0.0f;

	float Distance = bUseHorizontalDistanceOnly ? Origin->GetHorizontalDistanceTo(Target) : Origin->GetDistanceTo(Target);

	// Normalize the distance (0.0 to 1.0)
	// Closer = 1.0, Further = 0.0
	float NormalizedScore = 1.0f - FMath::Clamp(Distance / MaxDistance, 0.0f, 1.0f);

	return NormalizedScore;
}