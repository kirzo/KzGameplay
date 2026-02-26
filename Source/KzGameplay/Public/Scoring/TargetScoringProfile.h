// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "TargetScorer.h"
#include "TargetScoringProfile.generated.h"

/**
 * Defines a single scoring rule, applying a weight and an optional curve to a Scorer.
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FTargetScorerEntry
{
	GENERATED_BODY()

public:
	/** The logic used to evaluate the target. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Scoring")
	TObjectPtr<UTargetScorer> Scorer;

	/** Multiplier applied to the final score of this evaluator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float Weight = 1.0f;

	/**
	 * Optional curve to remap the raw score.
	 * If the curve has keys, the raw score (X axis) will be mapped to the curve's value (Y axis).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	FRuntimeFloatCurve ScoreCurve;
};

/**
 * A reusable profile containing multiple scoring entries.
 * Can be added as a property to any Component (like an Interactor or an Auto-Aim system).
 */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FTargetScoringProfile
{
	GENERATED_BODY()

public:
	/** The list of scorers that will contribute to the final score. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	TArray<FTargetScorerEntry> Entries;
};