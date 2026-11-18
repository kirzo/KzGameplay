// Copyright 2026 kirzo

#include "Combat/KzCombatSettings.h"
#include "Combat/KzCombatTags.h"

UKzCombatSettings::UKzCombatSettings()
{
	using namespace KzTags::Combat;
	HitDirections.Add(HitDirection_Thrust, FVector(1.0, 0.0, 0.0));
	HitDirections.Add(HitDirection_RightToLeft, FVector(1.0, -0.2, 0.0));
	HitDirections.Add(HitDirection_LeftToRight, FVector(1.0, 0.2, 0.0));
	HitDirections.Add(HitDirection_Overhead, FVector(0.0, 0.0, -1.0));
	HitDirections.Add(HitDirection_Uppercut, FVector(0.0, 0.0, 1.0));
}