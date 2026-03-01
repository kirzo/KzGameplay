// Copyright 2026 kirzo

#include "Scoring/KzTargetScoringLibrary.h"

float UKzTargetScoringLibrary::EvaluateTarget(const AActor* Origin, const AActor* Target, const FKzTargetScoringProfile& Profile)
{
	if (!Origin || !Target) return 0.0f;

	float FinalScore = 0.0f;

	for (const FKzTargetScorerEntry& Entry : Profile.Entries)
	{
		if (Entry.Scorer)
		{
			// 1. Get the raw score from the logic
			float RawScore = Entry.Scorer->CalculateScore(Origin, Target);

			// 2. Remap through the curve if it has data
			if (const FRichCurve* RichCurve = Entry.ScoreCurve.GetRichCurveConst())
			{
				if (!RichCurve->IsEmpty())
				{
					RawScore = RichCurve->Eval(RawScore);
				}
			}

			// 3. Apply the weight and accumulate
			FinalScore += (RawScore * Entry.Weight);
		}
	}

	return FinalScore;
}