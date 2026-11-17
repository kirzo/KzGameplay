// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KzCombatSettings.generated.h"

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
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UKzCombatSettings* Get() { return GetDefault<UKzCombatSettings>(); }

	/** Which way a hit shakes the mesh: along the hit, or as the first local player's camera sees it, so a hit into the screen still shows. */
	UPROPERTY(Config, EditAnywhere, Category = "Mesh Shake")
	EKzShakeSpace ShakeSpace = EKzShakeSpace::Hit;
};