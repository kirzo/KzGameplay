// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Combat/KzCombatTags.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "KzCombatTypes.generated.h"

class UKzItemDefinition;
class USceneComponent;

/** How a target takes a hit, by its defenses in this order. KzHitOutcome in Python, where KzLib's FKzHitResult is KzHitResult. */
UENUM(BlueprintType, meta = (ScriptName = "KzHitOutcome"))
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

/** Where a blow's direction comes from. */
UENUM(BlueprintType)
enum class EKzHitDirectionMode : uint8
{
	/** One of the project's Hit Directions, by tag, in Project Settings -> Plugins -> Kz Combat. */
	Preset,

	/** Along the hitbox's own motion as it hits, for hitboxes that follow the weapon. Straight through the target if it barely moves. */
	HitboxMotion UMETA(DisplayName = "Hitbox Motion"),

	/** Its own vector. */
	Custom
};

/** Only to load hit directions saved before the presets moved to Project Settings, as the preset each had then. */
UENUM()
enum class EKzHitDirectionPreset : uint8
{
	Thrust,
	RightToLeft,
	LeftToRight,
	Overhead,
	Uppercut,
	HitboxMotion,
	Custom,
	None
};

/** Which way a blow travels, which the target's reaction follows: the side it staggers from, how it turns, and where it is pushed. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzHitDirection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Direction")
	EKzHitDirectionMode Mode = EKzHitDirectionMode::Preset;

	/** The project's Hit Direction it plays with Mode Preset. One missing there plays straight through the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Direction", meta = (Categories = "HitDirection"))
	FGameplayTag PresetTag = KzTags::Combat::HitDirection_Thrust;

	/**
	 * Custom's own, in the hit's frame: X along the ground from the attacker to the target, Y to the attacker's right of that line, Z up. Only its
	 * direction counts. Picking a preset copies the preset's here, so Custom starts from it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Direction")
	FVector Vector = FVector::ForwardVector;

	/** The project's vector for a Hit Direction tag, or null if it has none. */
	static const FVector* FindPreset(const FGameplayTag& Tag);

	/** In the world, normalized, for a blow from Attacker on Target whose hitbox moved by Motion as it hit. */
	FVector ToWorld(const FVector& Attacker, const FVector& Target, const FVector& Motion) const;

	void PostSerialize(const FArchive& Ar);

private:
	UPROPERTY()
	EKzHitDirectionPreset Preset_DEPRECATED = EKzHitDirectionPreset::None;
};

template<>
struct TStructOpsTypeTraits<FKzHitDirection> : public TStructOpsTypeTraitsBase2<FKzHitDirection>
{
	enum
	{
		WithPostSerialize = true
	};
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

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FGameplayTag AttackerCue;

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