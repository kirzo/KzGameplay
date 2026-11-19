// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "KzCombatSubsystem.generated.h"

class APawn;
class UKzCombat;
class UKzCombatant;
class UKzCombatConfig;

/**
 * The combats of a world. A pawn fights in the one it was assigned, such as its encounter's, or else in the default combat, which uses the
 * config of Project Settings -> Plugins -> Kz Combat and runs while anyone is in it.
 * kz.Combat.DrawTickets shows each combat's tickets and, over each attacker, its attack and cooldowns; kz.Combat.GrantAttack gives an
 * attack without tickets.
 */
UCLASS()
class KZAI_API UKzCombatSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UKzCombatSubsystem* Get(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	UKzCombat* GetDefaultCombat();

	/** A combat of its own, such as an encounter's: it hands out attacks between its Start and End. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	UKzCombat* CreateCombat(UKzCombatConfig* Config);

	/** The pawn fights in this combat from now on, instead of the default one; none sends it back there. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	void AssignCombat(APawn* Pawn, UKzCombat* Combat);

	/** The combat the pawn fights in: the one assigned to it, else the default one. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	UKzCombat* GetCombatFor(APawn* Pawn);

	UKzCombatant* FindCombatant(const APawn* Pawn) const;

	/** The pawn can't be given that attack until it is unblocked, such as until a phase of the fight. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	void BlockAttack(APawn* Pawn, FGameplayTag Attack);

	UFUNCTION(BlueprintCallable, Category = "KzAI|Combat")
	void UnblockAttack(APawn* Pawn, FGameplayTag Attack);

	/** Every combat there is, the default one included once it exists. */
	void GetCombats(TArray<UKzCombat*>& OutCombats) const;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKzCombatSubsystem, STATGROUP_Tickables); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	UPROPERTY()
	TObjectPtr<UKzCombat> DefaultCombat;

	UPROPERTY()
	TArray<TObjectPtr<UKzCombat>> Combats;

	TMap<TWeakObjectPtr<APawn>, TWeakObjectPtr<UKzCombat>> Assigned;
};