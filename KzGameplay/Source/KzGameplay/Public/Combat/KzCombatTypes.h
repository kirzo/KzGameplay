// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "KzCombatTypes.generated.h"

/** How a target takes a hit, by its defenses in this order. */
UENUM(BlueprintType)
enum class EKzHitResult : uint8
{
	/** Invulnerable, against an attack that is not Undodgeable: it does not count. */
	Ignored,

	/** The target parried: the attacker reacts. */
	Parried,

	Blocked,

	/** A Guard Break attack against a block. */
	GuardBroken,

	Hit
};

/** Something an attack hit, once per Hit Window. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzComboHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FHitResult Hit;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	EKzHitResult Result = EKzHitResult::Hit;

	/** Times the weapon's base damage, as the attack sets it. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	float DamageMultiplier = 1.0f;

	/** The reaction the Hit Window asks for; none is Reaction.Hit. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FGameplayTag Reaction;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FGameplayTagContainer AttackTags;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FGameplayTag ImpactCue;

	/** Whether it reached the target, for the attack's Hit and Miss outputs. */
	bool Counts() const { return Result != EKzHitResult::Ignored; }
};