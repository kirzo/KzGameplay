// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzTargetScoringProfile.h"
#include "KzTargetScoringLibrary.generated.h"

/** What one entry of a profile gave a target: its scorer's raw score, and what it added after its curve and weight. */
struct FKzTargetScoreBreakdown
{
	float Raw = 0.0f;
	float Added = 0.0f;
};

/**
 * Utility library to evaluate targets using scoring profiles.
 */
UCLASS()
class KZGAMEPLAY_API UKzTargetScoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Evaluates a target against an origin using a specific scoring profile.
	 * @param Origin The actor looking for targets.
	 * @param Target The candidate actor to evaluate.
	 * @param Profile The scoring configuration.
	 * @return The final accumulated score (higher is better).
	 */
	UFUNCTION(BlueprintCallable, Category = "KzGameplay|Scoring")
	static float EvaluateTarget(const FKzTransformSource& Origin, const FKzTransformSource& Target, const FKzTargetScoringProfile& Profile);

	/** EvaluateTarget, also giving what each entry of the profile added, in its order, such as for a debugger. */
	static float EvaluateTargetEntries(const FKzTransformSource& Origin, const FKzTransformSource& Target, const FKzTargetScoringProfile& Profile, TArray<FKzTargetScoreBreakdown>* OutEntries);
};