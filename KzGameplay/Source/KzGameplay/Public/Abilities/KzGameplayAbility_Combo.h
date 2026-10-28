// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "Combat/KzCombatTypes.h"
#include "Components/KzComponentReference.h"
#include "Engine/EngineTypes.h"
#include "KzGameplayAbility_Combo.generated.h"

class UGameplayEffect;
class UKzComboGraph;
class UKzComboInstance;
class UKzItemDefinition;

/**
 * Plays the combo of the melee weapon in a slot, or its own without one. Starts it with the input that activated it, hands it the presses of the
 * combo's inputs while it runs, and applies what it hits. Ends with the combo.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_Combo : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_Combo();

protected:
	/** The slot whose item's Melee Weapon fragment gives the combo and its damage. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (Categories = "Equipment.Slot"))
	FGameplayTag WeaponSlot;

	/** The combo without a melee weapon in the slot, such as punches. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Unarmed")
	TObjectPtr<UKzComboGraph> UnarmedCombo;

	UPROPERTY(EditDefaultsOnly, Category = "Combo|Unarmed", meta = (ClampMin = "0"))
	float UnarmedDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo|Unarmed")
	TSubclassOf<UGameplayEffect> UnarmedDamageEffect;

	/** The avatar's sensor that finds targets; none only turns attacks to the direction pressed. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (AllowedClasses = "/Script/KzGameplay.KzSensorComponent"))
	FKzComponentReference TargetSensor;

	/** What hitboxes hit; empty hits every dynamic object. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	/** Abilities that may cut the combo only inside an attack's Cancel Windows, such as dodging and blocking. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	FGameplayTagContainer CancelableBy;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnInputPressed(FGameplayTag Input);

	UFUNCTION()
	void OnComboHit(const FKzComboHit& Hit);

	UFUNCTION()
	void OnComboEnded(bool bInterrupted);

	UFUNCTION()
	void OnCancelable(bool bCancelable);

	/** Where the avatar's movement input points. */
	FVector GetInputDirection() const;

	void SetCancelBlocked(bool bBlocked);

	UPROPERTY()
	TObjectPtr<UKzComboInstance> Instance;

	UPROPERTY()
	TObjectPtr<const UKzItemDefinition> Item;

	float BaseDamage = 0.0f;
	TSubclassOf<UGameplayEffect> DamageEffect;
	bool bCancelBlocked = false;
};