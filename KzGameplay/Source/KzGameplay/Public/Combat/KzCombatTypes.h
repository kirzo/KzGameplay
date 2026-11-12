// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "KzCombatTypes.generated.h"

class UKzItemDefinition;
class USceneComponent;

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

/** Common ways a blow travels, in its hit's frame: X along the ground from the attacker to the target, Y to the attacker's right of that line, Z up. */
UENUM(BlueprintType)
enum class EKzHitDirectionPreset : uint8
{
	/** Straight through the target, away from the attacker: a thrust or a kick. */
	Thrust,

	/** Across the target from the attacker's right to its left, which comes in on the target's left when they face each other. */
	RightToLeft UMETA(DisplayName = "Right To Left"),

	/** Across the target from the attacker's left to its right, which comes in on the target's right when they face each other. */
	LeftToRight UMETA(DisplayName = "Left To Right"),

	/** Downward, such as a blow from above. */
	Overhead,

	/** Upward. */
	Uppercut,

	/** Along the hitbox's own motion as it hits, for hitboxes that follow the weapon. A thrust if it barely moves. */
	HitboxMotion UMETA(DisplayName = "Hitbox Motion"),

	/** The vector as set. */
	Custom
};

/** Which way a blow travels, which the target's reaction follows: the side it staggers from, how it turns, and where it is pushed. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzHitDirection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Direction")
	EKzHitDirectionPreset Preset = EKzHitDirectionPreset::Thrust;

	/**
	 * In the hit's frame: X along the ground from the attacker to the target, Y to the attacker's right of that line, Z up. Only its direction counts. Used
	 * by Custom; a preset shows its own here and plays that.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Direction")
	FVector Vector = FVector::ForwardVector;

	/** A preset's vector in the hit's frame. Hitbox Motion and Custom have none of their own: a thrust. */
	static FVector GetPresetVector(EKzHitDirectionPreset InPreset);

	/** In the world, normalized, for a blow from Attacker on Target whose hitbox moved by Motion as it hit. */
	FVector ToWorld(const FVector& Attacker, const FVector& Target, const FVector& Motion) const;
};

/** What stopped a hit: the defender's item with the Defense fragment, and where it is in the world. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzHitDefense
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	TObjectPtr<const UKzItemDefinition> Item;

	/** The item's actor root or mesh, which the defense cues spawn on. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	TObjectPtr<USceneComponent> Component;
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

	/** Times the weapon's base damage: the attack's multiplier times its Hit Window's. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	float DamageMultiplier = 1.0f;

	/** The reaction the Hit Window asks for; none is Reaction.Hit. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FGameplayTag Reaction;

	/** The attack's and its Hit Window's. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FGameplayTagContainer AttackTags;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FGameplayTag ImpactCue;

	/** Which way the blow travels in the world, normalized, by its Hit Window's Direction. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FVector Direction = FVector::ZeroVector;

	/** How hard it pushes, as a multiplier on the Knockback Window of the target's reaction. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	float Knockback = 1.0f;

	/** Set when it was blocked or parried. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FKzHitDefense Defense;

	/** Whether it reached the target, for the attack's Hit and Miss outputs. */
	bool Counts() const { return Result != EKzHitResult::Ignored; }
};

/** A combo hit as the target data of a reaction event: the hit, which way the blow travels, and how hard it pushes. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzHitReactionTargetData : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()

	/** In the world, normalized. Zero has the blow come straight from the event's instigator. */
	UPROPERTY()
	FVector Direction = FVector::ZeroVector;

	UPROPERTY()
	float Knockback = 1.0f;

	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FKzHitReactionTargetData> : public TStructOpsTypeTraitsBase2<FKzHitReactionTargetData>
{
	enum
	{
		WithNetSerializer = true
	};
};