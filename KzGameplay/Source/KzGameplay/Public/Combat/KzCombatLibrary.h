// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Combat/KzCombatTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "KzCombatLibrary.generated.h"

class UGameplayEffect;

UCLASS()
class KZGAMEPLAY_API UKzCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * How Target takes a hit from Attacker: State.Invulnerable, State.Parrying and State.Blocking in that order, the last two only within the angle of an
	 * equipped item with the Defense fragment, which OutDefense names. Targets without an ability system are always hit.
	 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	static EKzHitResult ResolveHit(const AActor* Attacker, const AActor* Target, const FGameplayTagContainer& AttackTags, FKzHitDefense& OutDefense);

	/**
	 * Applies a combo hit as the combo ability does: the damage effect with BaseDamage times the hit's multiplier as Data.Damage, the reaction events
	 * to defender and attacker, the attack's Impact Cue on a hit, and the defending item's cues on a block or a parry.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	static void ApplyComboHit(AActor* Attacker, const FKzComboHit& Hit, TSubclassOf<UGameplayEffect> DamageEffect, float BaseDamage, const UObject* SourceObject);

	/** The side something at From is on, seen from Actor: Direction.Front, Back, Left or Right. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	static FGameplayTag GetDirectionTag(const AActor* Actor, const FVector& From);

	/** Whether From lies within Angle degrees in front of Actor, on the ground plane. */
	static bool IsInFront(const AActor& Actor, const FVector& From, float Angle);
};