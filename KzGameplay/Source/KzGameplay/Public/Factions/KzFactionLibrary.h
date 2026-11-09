// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Factions/KzFactionSettings.h"
#include "KzFactionLibrary.generated.h"

/** Factions as actors have them: from the UKzFactionComponent of their controller and their own. */
UCLASS()
class KZGAMEPLAY_API UKzFactionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** An actor's faction: its controller's, when something with a faction drives it, else its own. */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static FGameplayTag GetFaction(const AActor* Actor);

	/**
	 * How Actor sees Other: Actor's exception towards Other's faction, else Other's towards Actor's, else the settings. An actor's own
	 * exceptions come before its controller's. Every actor is friendly to itself.
	 */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static EKzAttitude GetAttitude(const AActor* Actor, const AActor* Other);
};
