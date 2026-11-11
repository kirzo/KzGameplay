// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Combat/KzCombatTypes.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Tickable.h"
#include "KzComboInstance.generated.h"

class FKzAnimationPlay;
class UKzAnimationNotifyState;
class UKzAnimNotifyState_ComboWindow;
class UKzAnimNotifyState_HitWindow;
class UKzComboGraph;
class UKzItemDefinition;
class UKzSensorComponent;
class USceneComponent;
class USkeletalMeshComponent;
struct FKzComboBranchContext;
struct FKzComboNode_Attack;
struct FKzShapeInstance;

/** Who plays a combo, and with what. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzComboContext
{
	GENERATED_BODY()

	/** The mesh the attacks play on, which the avatar's hitboxes hang from. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** Who attacks: their database answers By Query animations, their tags pick Switch on Tags cases, and their own attacks never hit them. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<AActor> User;

	/** The weapon's item, whose Database fragment answers By Query animations. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<const UKzItemDefinition> Item;

	/** The weapon in the world, which its hitboxes hang from: the root of its actor, or the mesh equipment spawned for it. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<USceneComponent> Weapon;

	/** What hitboxes hit; empty hits every dynamic object, such as pawns, physics bodies and destructibles. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	/** The target, for drivers that already know it; it takes the place of the user's Sensor.Targeting sensor. */
	UPROPERTY(BlueprintReadWrite, Category = "Combo")
	TObjectPtr<AActor> Target;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzOnComboHit, const FKzComboHit&, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzOnComboEnded, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzOnComboCancelable, bool, bCancelable);

/**
 * One play of a combo: it keeps the count and reads the graph. It plays each attack, turns it to its target, hears its windows, sweeps its hitboxes,
 * and chains the next attack when a press comes inside a Combo Window, or a moment before one opens. Whoever drives it, such as the combo ability,
 * an AI or a Blueprint, starts it, hands it presses, and hears what it hit and when it ended.
 */
UCLASS(BlueprintType)
class KZGAMEPLAY_API UKzComboInstance : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Starts the combo with the input that started it, playing the attack the Entry leads to for it. Direction is where the input points, for picking the target. Null when it leads nowhere. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	static UKzComboInstance* StartCombo(UKzComboGraph* Combo, const FKzComboContext& Context, FGameplayTag Input, FVector Direction);

	/** A press of one of the combo's inputs: chains the attack it leads to inside a Combo Window, or is kept a moment in case one opens. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void Press(FGameplayTag Input, FVector Direction);

	/** Stops the attack and ends the combo, interrupted. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void Stop(float BlendOut = 0.2f);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool IsRunning() const { return bRunning; }

	/** Whether a Cancel Window of the attack playing is open. */
	UFUNCTION(BlueprintPure, Category = "Combo")
	bool IsCancelable() const { return CancelWindows > 0; }

	/** Who the attack playing goes for, if anyone. */
	UFUNCTION(BlueprintPure, Category = "Combo")
	AActor* GetTarget() const { return Target.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FKzOnComboHit OnHit;

	/** The last attack ended with nothing chained, something cut it, or it was stopped. */
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FKzOnComboEnded OnEnded;

	/** A Cancel Window opened, or the last one closed. */
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FKzOnComboCancelable OnCancelable;

	virtual UWorld* GetWorld() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bRunning; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual TStatId GetStatId() const override;

	/**
	 * Where an attack looks for targets: its own shape or else the sensor's, at the sensor's offset from the user and turned with it to Direction,
	 * the direction pressed. False with neither.
	 */
	static bool GetTargetingShape(const FKzComboNode_Attack& Attack, const UKzSensorComponent* Sensor, const FTransform& UserTransform, const FVector& Direction, FKzShapeInstance& OutShape, FTransform& OutTransform);

private:
	/** Plays an attack node in place of the one playing; a node that plays nothing ends the combo. */
	bool PlayAttack(int32 NodeIndex, const FVector& Direction);

	/** Plays the attack the input leads to from the current one, through any branches. False when it leads nowhere. */
	bool Chain(FGameplayTag Input, const FVector& Direction);

	/** What branches choose by now: the latest Combo Window open, whether the attack hit, and the attacker's tags. */
	FKzComboBranchContext MakeBranchContext() const;

	void OnNotify(const UKzAnimationNotifyState* Notify, bool bBegin, float Duration);
	void OnAttackEnded(bool bInterrupted);

	/** Sweeps every hitbox of the attack from where it was to where it is. */
	void SweepHitboxes();

	/** The best target the targeting sensor finds with the attack's shape turned to Direction, or the context's own. */
	AActor* FindTarget(const FVector& Direction) const;

	/** Turns the attacker towards the target, or the direction pressed, until the first Hit Window opens. */
	void UpdateFacing(float DeltaTime);

	/** Closes in on the target while a Warp Window is open. */
	void UpdateWarp(float DeltaTime);

	/** Opens a Warp Window, or closes the open one with null: one with a Height takes the attacker off the ground, and closing it lands it. */
	void SetWarp(const UKzAnimationNotifyState* Notify, float Duration);

	/** Freezes attacker and target for the hit's Hit Stop, the attack's or the defending item's, and slows a parried attacker. */
	void DilateTime(const FKzComboHit& Hit, AActor& HitActor);

	void SetCancelWindows(int32 Count);

	/**
	 * Opens or closes a Combo Window, with State.ComboWindow on the attacker while any is open and the window's name while it is, so whoever
	 * drives it knows when a press chains, and which way.
	 */
	void SetComboWindow(const UKzAnimNotifyState_ComboWindow& Window, bool bOpen);

	/** Closes every Combo Window still open, as a new attack or the end of the combo does. */
	void CloseComboWindows();

	void End(bool bInterrupted);

	const FKzComboNode_Attack* GetAttack() const;

	UPROPERTY()
	TObjectPtr<UKzComboGraph> Combo;

	UPROPERTY()
	FKzComboContext Context;

	/** The user's Sensor.Targeting sensor. */
	TWeakObjectPtr<UKzSensorComponent> Sensor;

	TSharedPtr<FKzAnimationPlay> Play;
	int32 Node = INDEX_NONE;
	bool bRunning = false;

	/** The attacker's own time, which stops with it during a Hit Stop. */
	double Clock = 0.0;

	/** The attack playing has hit something. */
	bool bHit = false;

	/** The Combo Windows open, the latest last: a press counts in that one. */
	TArray<TWeakObjectPtr<const UKzAnimNotifyState_ComboWindow>> ComboWindows;
	int32 HitWindows = 0;
	int32 CancelWindows = 0;

	/** The attack playing opened a Hit Window, which ends its turning. */
	bool bOpenedHitWindow = false;

	/** The last Hit Window that opened, whose reaction, damage and tags its hits take. */
	TWeakObjectPtr<const UKzAnimNotifyState_HitWindow> HitWindow;

	/** A press that came with no Combo Window open, and when, in case one opens within the combo's InputBuffer. */
	FGameplayTag KeptInput;
	FVector KeptDirection = FVector::ZeroVector;
	double KeptAt = 0.0;

	TWeakObjectPtr<AActor> Target;

	/** Where the attack playing turns to without a target. */
	FVector FacingDirection = FVector::ZeroVector;

	/** The Warp Window open, if any, how long it lasts, how long it has left and how far it moved, and whether its arc has the attacker in the air. */
	TWeakObjectPtr<const UKzAnimationNotifyState> Warp;
	float WarpDuration = 0.0f;
	float WarpTimeLeft = 0.0f;
	float WarpMoved = 0.0f;
	bool bWarpAirborne = false;

	/** Per hitbox, where what it hangs from was on the last sweep. */
	TArray<TOptional<FTransform>> PreviousParents;

	/** What the Hit Window has hit, so each target takes one hit. */
	TSet<TWeakObjectPtr<AActor>> HitActors;
};