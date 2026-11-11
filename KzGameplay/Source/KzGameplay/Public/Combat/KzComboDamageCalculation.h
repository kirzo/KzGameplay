// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "KzComboDamageCalculation.generated.h"

/**
 * A combo hit's damage, as the combo sends it in Data.Damage, for a damage effect's modifier. Unlike Set by Caller, the modifier's Coefficient
 * scales it, such as half damage in a modifier whose Target Tags require State.SuperArmor.
 */
UCLASS(DisplayName = "Combo Damage")
class KZGAMEPLAY_API UKzComboDamageCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};