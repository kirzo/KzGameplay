// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Misc/KzTransformSource.h"
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
	 * @return The raw score calculated by this specific logic.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Scoring")
	float CalculateScore(const FKzTransformSource& Origin, const FKzTransformSource& Target) const;

protected:
	virtual float CalculateScore_Implementation(const FKzTransformSource& Origin, const FKzTransformSource& Target) const { return 0.0f; }
};