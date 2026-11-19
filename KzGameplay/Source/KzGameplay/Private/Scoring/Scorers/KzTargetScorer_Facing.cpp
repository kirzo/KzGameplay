// Copyright 2026 kirzo

#include "Scoring/Scorers/KzTargetScorer_Facing.h"
#include "Interaction/KzInteractableComponent.h"

float UKzTargetScorer_Facing::CalculateScore(const FKzTransformSource& Origin, const FKzTransformSource& Target) const
{
	if (!Origin.IsValid() || !Target.IsValid()) return 0.0f;

	// What is reached from any side has no front: scored by where its forward happens to point, it would lose to chance
	const UKzInteractableComponent* Interactable = Cast<UKzInteractableComponent>(Target.GetSceneComponent());
	if (Interactable && !Interactable->bLimitApproach)
	{
		return 1.0f;
	}

	const FTransform TargetTransform = Target.GetTransform();
	const FVector ToOrigin = (Origin.GetLocation() - TargetTransform.GetLocation()).GetSafeNormal();
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(TargetTransform.GetRotation().GetForwardVector() | ToOrigin), -1.0f, 1.0f)));

	// Normalize the score (0.0 to 1.0)
	return 1.0f - FMath::Clamp(Angle / MaxAngle, 0.0f, 1.0f);
}