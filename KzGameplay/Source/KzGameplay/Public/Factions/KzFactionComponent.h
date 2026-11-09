// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Factions/KzFactionSettings.h"
#include "KzFactionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzFactionChangedSignature, FGameplayTag, Faction);

/**
 * A faction and its exceptions to the settings' relations.
 *
 * On a controller, player or AI, it gives its faction to the pawn it drives, so the same character can be an ally or an enemy by who
 * drives it, and a player can change sides. On any other actor, it gives the faction of an actor nobody drives, and the exceptions of
 * that one instance, which come before its controller's. UKzFactionLibrary puts both together.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzFactionComponent();

	UFUNCTION(BlueprintPure, Category = "Faction")
	FGameplayTag GetFaction() const { return Faction; }

	UFUNCTION(BlueprintCallable, Category = "Faction")
	void SetFaction(UPARAM(meta = (Categories = "Faction")) FGameplayTag NewFaction);

	/** Sees a faction otherwise than the settings say, such as a crow friendly to the player's faction. */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	void SetAttitudeTowards(UPARAM(meta = (Categories = "Faction")) FGameplayTag OtherFaction, EKzAttitude Attitude);

	UFUNCTION(BlueprintCallable, Category = "Faction")
	void ClearAttitudeTowards(UPARAM(meta = (Categories = "Faction")) FGameplayTag OtherFaction);

	/** The exception towards a faction, if there is one. */
	bool FindAttitudeTowards(const FGameplayTag& OtherFaction, EKzAttitude& OutAttitude) const;

	UPROPERTY(BlueprintAssignable, Category = "Faction")
	FKzFactionChangedSignature OnFactionChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (Categories = "Faction"))
	FGameplayTag Faction;

	/** Exceptions to the settings, from this side: how it sees each of these factions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (Categories = "Faction"))
	TMap<FGameplayTag, EKzAttitude> Attitudes;
};
