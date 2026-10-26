// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Tickable.h"
#include "KzComboInstance.generated.h"

class FKzAnimationPlay;
class UKzAnimationNotifyState;
class UKzComboGraph;
class UKzItemDefinition;
class USceneComponent;
class USkeletalMeshComponent;
struct FKzComboNode_Attack;

/** Who plays a combo, and with what. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzComboContext
{
	GENERATED_BODY()

	/** The mesh the attacks play on, which the avatar's hitboxes hang from. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** Who attacks: their database answers By Query animations, and their own attacks never hit them. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<AActor> User;

	/** The weapon's item, whose Database fragment answers By Query animations. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UKzItemDefinition> Item;

	/** The weapon in the world, which its hitboxes hang from: the root of its actor, or the mesh equipment spawned for it. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<USceneComponent> Weapon;

	/** What hitboxes hit; empty hits every dynamic object, such as pawns, physics bodies and destructibles. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
};

/** Something an attack hit, once per attack. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzComboHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	FHitResult Hit;

	/** Times the weapon's base damage, as the attack sets it. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	float DamageMultiplier = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzOnComboHit, const FKzComboHit&, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzOnComboEnded, bool, bInterrupted);

/**
 * One play of a combo: it keeps the count and reads the graph. It plays each attack, hears its Hit and Combo Windows, sweeps its hitboxes,
 * and chains the next attack when a press comes inside a Combo Window, or a moment before one opens. Whoever drives it, such as the combo ability,
 * an AI or a Blueprint, starts it, hands it presses, and hears what it hit and when it ended.
 */
UCLASS(BlueprintType)
class KZGAMEPLAY_API UKzComboInstance : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Starts the combo with the input that started it, playing the attack the Entry leads to for it. Null when it leads nowhere. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	static UKzComboInstance* StartCombo(UKzComboGraph* Combo, const FKzComboContext& Context, FGameplayTag Input);

	/** A press of one of the combo's inputs: chains the attack it leads to inside a Combo Window, or is kept a moment in case one opens. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void Press(FGameplayTag Input);

	/** Stops the attack and ends the combo, interrupted. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void Stop(float BlendOut = 0.2f);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool IsRunning() const { return bRunning; }

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FKzOnComboHit OnHit;

	/** The last attack ended with nothing chained, something cut it, or it was stopped. */
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FKzOnComboEnded OnEnded;

	virtual UWorld* GetWorld() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bRunning; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual TStatId GetStatId() const override;

private:
	/** Plays an attack node in place of the one playing; a node that plays nothing ends the combo. */
	bool PlayAttack(int32 NodeIndex);

	/** Plays the attack the input leads to from the current one, given whether it hit. False when it leads nowhere. */
	bool Chain(FGameplayTag Input);

	void OnNotify(const UKzAnimationNotifyState* Notify, bool bBegin);
	void OnAttackEnded(bool bInterrupted);

	/** Sweeps every hitbox of the attack from where it was to where it is. */
	void SweepHitboxes();

	void End(bool bInterrupted);

	const FKzComboNode_Attack* GetAttack() const;

	UPROPERTY()
	TObjectPtr<UKzComboGraph> Combo;

	UPROPERTY()
	FKzComboContext Context;

	TSharedPtr<FKzAnimationPlay> Play;
	int32 Node = INDEX_NONE;
	bool bRunning = false;

	/** The attack playing has hit something. */
	bool bHit = false;

	bool bComboWindow = false;
	int32 HitWindows = 0;

	/** A press that came with no Combo Window open, and when, in case one opens within the combo's InputBuffer. */
	FGameplayTag KeptInput;
	double KeptAt = 0.0;

	/** Per hitbox, where what it hangs from was on the last sweep. */
	TArray<TOptional<FTransform>> PreviousParents;

	/** What the attack playing has hit, so each target takes one hit. */
	TSet<TWeakObjectPtr<AActor>> HitActors;
};