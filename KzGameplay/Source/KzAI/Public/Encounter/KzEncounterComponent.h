// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/TimerHandle.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTagContainer.h"
#include "ScriptableConditions/ScriptableRequirement.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "Sensors/KzSensorComponent.h"
#include "KzEncounterComponent.generated.h"

class UAbilitySystemComponent;
class UKzCombat;
class UKzCombatant;
class UKzCombatConfig;
class UKzEngagementComponent;
class UKzEngagementConfig;
class UKzSpawnComponent;

UENUM(BlueprintType)
enum class EKzEncounterState : uint8
{
	Waiting,
	Active,
	Ended
};

/** Enemies of one class that a wave brings through one spawn point. */
USTRUCT(BlueprintType)
struct KZAI_API FKzEncounterSpawn
{
	GENERATED_BODY()

	/** A Spawn Point, or any actor with a Spawn Component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TObjectPtr<AActor> SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<AActor> Class;

	/** They come together, and the point finds room for each. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** From the start of its wave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0", Units = "Seconds"))
	float Delay = 0.0f;

	/** What its conditions tell these enemies apart by, such as Encounter.Boss. The enemies themselves need nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	FGameplayTagContainer Tags;
};

USTRUCT(BlueprintType)
struct KZAI_API FKzEncounterWave
{
	GENERATED_BODY()

	FKzEncounterWave();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FKzEncounterSpawn> Spawns;

	/** When it comes. Empty, once every enemy before it is defeated. Its context has the Encounter and its Target. */
	UPROPERTY(EditAnywhere, Category = "Wave")
	FScriptableRequirement StartWhen;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzEncounterStartedSignature, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzEncounterWaveSignature, int32, Wave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKzEncounterEndedSignature);

/**
 * A fight in one place: enemies already there and waves that come through spawn points, fighting in a combat of their own.
 * It starts when its Sensor.Encounter sensor senses something, when one of its enemies engages, or from Blueprint, and ends once every
 * enemy is defeated, when End When is met, or from Blueprint. Its Limits are only on while it is active. An enemy is defeated once it
 * has State.Dead or is destroyed. LogKzEncounter tells what it does; kz.Encounter.Draw shows its state over it.
 */
UCLASS(ClassGroup = (KzAI), meta = (BlueprintSpawnableComponent))
class KZAI_API UKzEncounterComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UKzEncounterComponent();

	/** The config of its combat. None takes the default one of Project Settings -> Plugins -> Kz Combat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UKzCombatConfig> CombatConfig;

	/** Where its enemies place themselves around their targets while it lasts. None leaves the targets' own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UKzEngagementConfig> EngagementConfig;

	/** The enemies already there before it starts: placed in the level, or spawn points whose Auto Spawn counts as its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemies")
	TArray<TObjectPtr<AActor>> Initial;

	/** In order; with Initial enemies, those are the first wave. */
	UPROPERTY(EditAnywhere, Category = "Enemies")
	TArray<FKzEncounterWave> Waves;

	/** How many can be alive at once; the rest wait for room. 0 has no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemies", meta = (ClampMin = "0"))
	int32 MaxAlive = 0;

	/** Starts when its owner's Sensor.Encounter sensor senses something, such as the player walking in. That sensor turns off then. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start")
	bool bStartOnSensor = false;

	/**
	 * Starts when one of its enemies engages a target, such as one that sees the player or is attacked, or is defeated before it could:
	 * then against whoever last attacked one of its enemies.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start")
	bool bStartOnEngage = true;

	/** Runs as it starts, such as the combat camera. Its context has the Encounter and its Target. */
	UPROPERTY(EditAnywhere, Category = "Start", meta = (DisplayName = "Start Action"))
	FScriptableAction StartAction;

	/** Ends it before every enemy is defeated. Its context has the Encounter and its Target. */
	UPROPERTY(EditAnywhere, Category = "End")
	FScriptableRequirement EndWhen;

	/** From End When being met to the end. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "End", meta = (ClampMin = "0", Units = "Seconds"))
	float EndDelay = 0.0f;

	/** Runs as it ends, whatever ends it. Its context has the Encounter and its Target. */
	UPROPERTY(EditAnywhere, Category = "End", meta = (DisplayName = "End Action"))
	FScriptableAction EndAction;

	/** Only there while it is active. Kz Toggleable actors switch themselves, such as camera limits; others only show and collide then, such as blocking volumes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits")
	TArray<TObjectPtr<AActor>> Limits;

	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FKzEncounterStartedSignature OnStarted;

	/** One of its Waves came, by its index. */
	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FKzEncounterWaveSignature OnWaveStarted;

	/** Whatever ended it. */
	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FKzEncounterEndedSignature OnEnded;

	/** Starts it against Target, whom its enemies go for while they perceive nobody. Only while waiting. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void Start(AActor* InTarget);

	/** Ends it now. Its enemies left stay as they are. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void End();

	UFUNCTION(BlueprintPure, Category = "Encounter")
	EKzEncounterState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	AActor* GetTarget() const { return Target.Get(); }

	/** How many of its Waves have come. */
	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetWavesStarted() const { return NextWave; }

	/** How many of its Waves have come and been defeated whole; the Initial enemies aren't one. */
	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetWavesCleared() const;

	/** Its enemies alive, those with any of Tags if there are some. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void GetEnemies(const FGameplayTagContainer& Tags, TArray<AActor*>& OutEnemies) const;

	/** Its enemies alive or on their way: all, or of the last of its Waves to come (the Initial ones before any), of Tags if there are some. */
	int32 GetAliveCount(bool bLastWave, const FGameplayTagContainer& Tags) const;

	/** Its enemies defeated since the last of its Waves came, or since it started, of Tags if there are some. */
	int32 GetDefeatedCount(bool bSinceWave, const FGameplayTagContainer& Tags) const;

	/** Seconds since the last of its Waves came, or since it started. */
	double GetTimeSince(bool bSinceWave) const;

	UKzCombat* GetCombat() const { return Combat; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FEnemy
	{
		TWeakObjectPtr<AActor> Actor;
		FGameplayTagContainer Tags;

		/** Its index in Waves; INDEX_NONE for the Initial ones. */
		int32 Wave = INDEX_NONE;

		TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
		FDelegateHandle DeadHandle;
	};

	/** One enemy of a wave waiting to be asked for. */
	struct FQueued
	{
		int32 Wave = 0;
		int32 Spawn = 0;
		double Time = 0.0;
	};

	EKzEncounterState State = EKzEncounterState::Waiting;
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(Transient)
	TObjectPtr<UKzCombat> Combat;

	TArray<FEnemy> Enemies;
	TArray<FQueued> Queue;

	/** The tags of each enemy defeated, in order; a defeat counts after a wave or the start by where it is, not by its time, as the same frame can hold both. */
	TArray<FGameplayTagContainer> Defeats;
	int32 StartDefeat = 0;
	int32 WaveDefeat = 0;

	/** Asked of their spawn points and on their way. */
	TArray<FQueued> InFlight;

	/** Per wave, its enemies not spawned yet: queued, or asked for and on their way. */
	TArray<int32> Unspawned;
	int32 NextWave = 0;

	double StartTime = 0.0;
	double WaveStartTime = 0.0;

	FTimerHandle UpdateTimer;
	FTimerHandle EndTimer;
	bool bUpdateRequested = false;

	TWeakObjectPtr<UKzSensorComponent> StartSensor;

	/** Whoever last picked one of its enemies as an attack's target or hit it, to start against if one falls while it waits. */
	TWeakObjectPtr<AActor> LastAttacker;
	TArray<TWeakObjectPtr<UKzSpawnComponent>> InitialPoints;
	TArray<TWeakObjectPtr<UKzEngagementComponent>> Overridden;

	/** Waves start, queued enemies are asked for, and it ends if it should. */
	void Update();

	/** Update on the next tick, out of whatever callback asks for it. */
	void RequestUpdate();

	bool CanStartWave(int32 Wave);
	void StartWave(int32 Wave);
	void SpawnQueued(const FQueued& Queued);
	void HandleWaveSpawned(AActor* Actor, int32 Wave, int32 Spawn);

	void AddEnemy(AActor* Actor, const FGameplayTagContainer& Tags, int32 Wave);
	FEnemy* FindEnemy(const AActor* Actor);
	void Defeat(AActor* Actor);
	void StopWatching(const FEnemy& Enemy);
	void SuggestTarget(AActor* Enemy) const;

	void SetLimitsEnabled(bool bEnabled);
	void OverrideEngagement(AActor* EngagedTarget);
	void RestoreEngagements();

	bool Evaluate(const FScriptableRequirement& Requirement);
	void RunAction(const FScriptableAction& Action);

	void HandleCombatantJoined(UKzCombatant* Combatant);
	void HandleDeadTag(const FGameplayTag Tag, int32 Count, TWeakObjectPtr<AActor> Actor);

	UFUNCTION()
	void HandleSensed(const FKzOverlapResult& OverlapInfo);

	UFUNCTION()
	void HandleInitialSpawned(AActor* Actor);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleEnemyEvent(const FGameplayTag& Tag, FGameplayEventData EventData);
};