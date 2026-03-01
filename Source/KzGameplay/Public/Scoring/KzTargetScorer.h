// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KzTargetScorer.generated.h"

/**
 * Base class for all modular scorers.
 * Evaluates a specific relationship between an Origin and a Target and returns a raw score.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class KZGAMEPLAY_API UKzTargetScorer : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Calculates the raw score for the given Target relative to the Origin.
	 * Normally returns a normalized value between 0.0 and 1.0, but any range is valid.
	 * @param Origin The actor performing the evaluation (e.g., the Player).
	 * @param Target The actor being evaluated (e.g., an interactable object or an enemy).
	 * @return The raw score calculated by this specific logic.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Scoring")
	float CalculateScore(const AActor* Origin, const AActor* Target) const;

protected:
	virtual float CalculateScore_Implementation(const AActor* Origin, const AActor* Target) const { return 0.0f; }
};