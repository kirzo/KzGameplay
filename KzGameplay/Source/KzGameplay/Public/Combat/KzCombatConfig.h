// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Tickets/KzTicketDispatcher.h"
#include "Tickets/KzTicketPool.h"
#include "KzCombatConfig.generated.h"

/**
 * How a combat hands out attacks among its attackers: the tickets it has, how often and to how many it gives them, and who can't take any.
 * One is the project's default, under Project Settings -> Plugins -> Kz Combat; an encounter can bring its own.
 */
UCLASS(BlueprintType)
class KZGAMEPLAY_API UKzCombatConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UKzCombatConfig();

	/** Each type of ticket an attack can take: how many there are, and how long they cool down once given back. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tickets")
	FKzTicketPoolSettings Pool;

	/** How often each type is handed out, after an initial delay, to how many attackers at once, and how they are chosen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tickets")
	FKzTicketDispatcherSettings Dispatcher;

	/** When the combat starts, every ticket and every attack starts cooling down, so it opens calmly. Best with a short Initial Delay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start")
	bool bCooldownOnStart = false;

	/** An attacker with any of these can't be given an attack, such as one already attacking, reacting to a hit or dead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attackers")
	FGameplayTagContainer BlockedTags;
};