// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Combat/KzAttackSet.h"
#include "Tickets/KzTicketDispatcher.h"
#include "Tickets/KzTicketPool.h"
#include "KzCombat.generated.h"

class APawn;
class UAbilitySystemComponent;
class UKzCombat;
class UKzCombatConfig;

/**
 * An attacker in a combat: the attacks it can be given, their cooldowns and which are blocked, and the attack it was given, from when it
 * gets it until its tree is done with it. Its combat's dispatcher asks it what it could do and hands it attacks.
 */
UCLASS()
class KZAI_API UKzCombatant : public UObject, public IKzTicketParticipant
{
	GENERATED_BODY()

public:
	APawn* GetPawn() const { return Pawn.Get(); }
	AActor* GetTarget() const { return Target.Get(); }
	const UKzAttackSet* GetAttackSet() const { return AttackSet; }
	UKzCombat* GetCombat() const;

	/** The attack it was given, from when it gets it until it is over; none otherwise. */
	FGameplayTag GetGrantedAttack() const { return GrantedAttack; }

	/** It was given an attack its tree hasn't taken yet. */
	bool HasAttackToTake() const { return GrantedAttack.IsValid() && !bTaken; }

	/** The attack it was given, for its tree to make; under way from then on. False without one. */
	bool TakeAttack(FKzAttackDefinition& OutAttack);

	/**
	 * The attack is over. Started and not cut short by a hit, it cools down for its Cooldown and its tickets for theirs; cut short, for its
	 * Cancelled Cooldown. Never started, such as when it couldn't get close, its tickets go back free and are handed out again at once.
	 */
	void FinishAttack();

	/** Gives it an attack, with its tickets or without any, such as from the console. False if it has one already or doesn't know it. */
	bool GrantAttack(FGameplayTag Attack, const FKzTicketHandle& Tickets);

	/** The attacks it could be given right now, whatever their tickets. */
	void GetAvailableAttacks(TArray<FGameplayTag>& OutAttacks) const;

	void SetBlocked(FGameplayTag Attack, bool bBlocked);
	bool IsBlocked(FGameplayTag Attack) const { return Blocked.Contains(Attack); }

	/** How long until an attack can be given again. */
	float GetCooldownLeft(FGameplayTag Attack) const;

	/** Every attack starts its Cooldown now. */
	void StartCooldowns();

	virtual UWorld* GetWorld() const override;

	virtual void GetTicketOptions_Implementation(FGameplayTag Type, TArray<FKzTicketOption>& Options) override;
	virtual void OnTicketsGranted_Implementation(const FKzTicketOption& Option, const FKzTicketHandle& Handle) override;

private:
	friend class UKzCombat;

	/** Where the attacker stands around its target, for the areas attacks can be made from. */
	struct FArea
	{
		bool bKnown = false;
		bool bForbidden = false;
		bool bOutside = false;
		FGameplayTag Tag;
	};

	void Setup(APawn* InPawn, AActor* InTarget, UKzAttackSet* InAttackSet);
	FArea GetArea() const;
	bool CanMake(const FGameplayTag& Attack, const FKzAttackDefinition& Definition, const FArea& Area) const;
	bool IsReady() const;
	void OnTagChanged(const FGameplayTag Tag, int32 Count);
	void StopWatching();
	double GetTime() const;

	TWeakObjectPtr<APawn> Pawn;
	TWeakObjectPtr<AActor> Target;

	UPROPERTY()
	TObjectPtr<UKzAttackSet> AttackSet;

	/** When each attack can be given again. */
	TMap<FGameplayTag, double> ReadyTimes;

	TSet<FGameplayTag> Blocked;

	FGameplayTag GrantedAttack;
	FKzTicketHandle Tickets;

	/** Its tree took the attack. */
	bool bTaken = false;

	/** It started attacking, and then a hit cut it short. */
	bool bStarted = false;
	bool bInterrupted = false;

	/** Offers nothing, while its own given-back tickets are handed out again. */
	bool bStandingDown = false;

	TWeakObjectPtr<UAbilitySystemComponent> WatchedAbilitySystem;
	FDelegateHandle AttackingHandle;
	FDelegateHandle ReactingHandle;
};

/**
 * Attackers fighting together, sharing the tickets that pace their attacks: a pool of them and a dispatcher that hands them out, as its
 * config says. Its combatants can attack different targets; each places itself around its own. One with auto start begins with its first
 * combatant and ends with its last; otherwise whoever made it, such as an encounter, starts and ends it.
 */
UCLASS(BlueprintType)
class KZAI_API UKzCombat : public UObject
{
	GENERATED_BODY()

public:
	void Setup(UKzCombatConfig* InConfig, bool bInAutoStart);

	/** Another config while it runs; combatants and the tickets they hold stay. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	void ApplyConfig(UKzCombatConfig* NewConfig);

	const UKzCombatConfig* GetConfig() const;

	/** Starts handing out attacks: tickets as the config sets them, after its initial delay. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	void Start();

	/** Stops handing out attacks. Attacks already given go on. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	void End();

	UFUNCTION(BlueprintPure, Category = "KzAI|Combat")
	bool IsActive() const { return bActive; }

	/** Adds the pawn with its attacks against its target, or updates them if it is in already. */
	UKzCombatant* Join(APawn* Pawn, AActor* Target, UKzAttackSet* Attacks);

	/** Takes the pawn out, ending an attack it may have. */
	void Leave(APawn* Pawn);

	UKzCombatant* FindCombatant(const APawn* Pawn) const;
	const TArray<TObjectPtr<UKzCombatant>>& GetCombatants() const { return Combatants; }

	UKzTicketPool* GetPool() const { return Pool; }
	UKzTicketDispatcher* GetDispatcher() const { return Dispatcher; }

	virtual UWorld* GetWorld() const override;

private:
	UPROPERTY()
	TObjectPtr<UKzCombatConfig> Config;

	UPROPERTY()
	TObjectPtr<UKzTicketPool> Pool;

	UPROPERTY()
	TObjectPtr<UKzTicketDispatcher> Dispatcher;

	UPROPERTY()
	TArray<TObjectPtr<UKzCombatant>> Combatants;

	bool bAutoStart = false;
	bool bActive = false;
};