// Copyright 2026 kirzo

#include "Combat/KzComboDamageCalculation.h"
#include "Combat/KzCombatTags.h"
#include "GameplayEffect.h"

float UKzComboDamageCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return Spec.GetSetByCallerMagnitude(KzTags::Combat::Data_Damage, false);
}