// Copyright 2026 kirzo

#include "Scoring/KzTargetScoringLibrary.h"

float UKzTargetScoringLibrary::EvaluateTarget(const FKzTransformSource& Origin, const FKzTransformSource& Target, const FKzTargetScoringProfile& Profile)
{
	return EvaluateTargetEntries(Origin, Target, Profile, nullptr);
}

float UKzTargetScoringLibrary::EvaluateTargetEntries(const FKzTransformSource& Origin, const FKzTransformSource& Target, const FKzTargetScoringProfile& Profile, TArray<FKzTargetScoreBreakdown>* OutEntries)
{
	if (OutEntries)
	{
		OutEntries->Reset();
	}
	if (!Origin.IsValid() || !Target.IsValid()) return 0.0f;

	float FinalScore = 0.0f;

	for (const FKzTargetScorerEntry& Entry : Profile.Entries)
	{
		FKzTargetScoreBreakdown Breakdown;
		if (Entry.Scorer)
		{
			// 1. Get the raw score from the logic
			Breakdown.Raw = Entry.Scorer->CalculateScore(Origin, Target);
			float Score = Breakdown.Raw;

			// 2. Remap through the curve if it has data
			if (const FRichCurve* RichCurve = Entry.ScoreCurve.GetRichCurveConst())
			{
				if (!RichCurve->IsEmpty())
				{
					Score = RichCurve->Eval(Score);
				}
			}

			// 3. Apply the weight and accumulate
			Breakdown.Added = Score * Entry.Weight;
			FinalScore += Breakdown.Added;
		}
		if (OutEntries)
		{
			OutEntries->Add(Breakdown);
		}
	}

	return FinalScore;
}