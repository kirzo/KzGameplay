// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TargetScoringProfile.h"
#include "TargetScoringLibrary.generated.h"

/**
 * Utility library to evaluate targets using scoring profiles.
 */
UCLASS()
class KZGAMEPLAY_API UTargetScoringLibrary : public UBlueprintFunctionLibrary
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
	static float EvaluateTarget(const AActor* Origin, const AActor* Target, const FTargetScoringProfile& Profile);
};