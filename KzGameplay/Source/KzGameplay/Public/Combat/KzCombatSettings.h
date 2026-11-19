// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "KzCombatSettings.generated.h"

class UKzCombatConfig;

/** Which way a hit shakes a mesh. */
UENUM(BlueprintType)
enum class EKzShakeSpace : uint8
{
	/** Along the hit. */
	Hit,

	/** Across the screen, along the camera's right, whichever way the hit goes; it starts on the side the hit leans to. */
	ScreenHorizontal,

	/** Along the hit as the screen shows it, without its depth: an overhead still shakes up and down, and a hit almost all into the screen goes across it. */
	ScreenProjected
};

/** Combat settings, under Project Settings -> Plugins -> Kz Combat. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Kz Combat"))
class KZGAMEPLAY_API UKzCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UKzCombatSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UKzCombatSettings* Get() { return GetDefault<UKzCombatSettings>(); }

	/** Attacks hurt those whose faction their attacker sees as friendly too. Off, a hitbox passes through allies; neutrals and enemies are hit either way. */
	UPROPERTY(Config, EditAnywhere, Category = "Hits")
	bool bFriendlyFire = false;

	/**
	 * The ways a blow can travel that Hit Windows pick by tag, in the hit's frame: X along the ground from the attacker to the target, Y to the
	 * attacker's right of that line, Z up. Only the direction counts, so (1, -0.2, 0) pushes mostly away and a little to the attacker's left.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Hit Directions", meta = (Categories = "HitDirection", ForceInlineRow))
	TMap<FGameplayTag, FVector> HitDirections;

	/** Which way a hit shakes the mesh: along the hit, or as the first local player's camera sees it, so a hit into the screen still shows. */
	UPROPERTY(Config, EditAnywhere, Category = "Mesh Shake")
	EKzShakeSpace ShakeSpace = EKzShakeSpace::Hit;

	/** How a combat hands out attacks when nothing brings its own, such as enemies fighting outside an encounter. */
	UPROPERTY(Config, EditAnywhere, Category = "Attack Tickets")
	TSoftObjectPtr<UKzCombatConfig> DefaultCombatConfig;
};