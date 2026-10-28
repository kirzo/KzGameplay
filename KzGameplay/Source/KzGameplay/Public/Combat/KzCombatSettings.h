// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "KzCombatSettings.generated.h"

/** How combo hits meet defenses, project wide. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kz Combat"))
class KZGAMEPLAY_API UKzCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UKzCombatSettings();

	/** How wide in front of a blocking character hits are blocked. */
	UPROPERTY(Config, EditAnywhere, Category = "Defense", meta = (ClampMin = "0", ClampMax = "360", Units = "Degrees"))
	float BlockAngle = 120.0f;

	/** How wide in front of a parrying character hits are parried. */
	UPROPERTY(Config, EditAnywhere, Category = "Defense", meta = (ClampMin = "0", ClampMax = "360", Units = "Degrees"))
	float ParryAngle = 120.0f;

	/** Executed on the defender when it blocks or its guard breaks, such as sparks off a shield. */
	UPROPERTY(Config, EditAnywhere, Category = "Feedback", meta = (Categories = "GameplayCue"))
	FGameplayTag BlockCue;

	/** Executed on the defender when it parries. */
	UPROPERTY(Config, EditAnywhere, Category = "Feedback", meta = (Categories = "GameplayCue"))
	FGameplayTag ParryCue;
};