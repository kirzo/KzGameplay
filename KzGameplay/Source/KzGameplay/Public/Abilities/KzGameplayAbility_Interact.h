// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "KzGameplayAbility_Interact.generated.h"

class UKzInteractorComponent;

/**
 * Interacts with what the avatar's interactor focuses, on Input.Interact, and ends at once: an interaction that lasts lives on in the
 * subsystem, and what goes with it triggers on Interaction.Begun instead. Granted by Capability.Interact. Picking up an item is the item's
 * own answer to the interaction.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_Interact : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_Interact();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	static UKzInteractorComponent* GetInteractor(const FGameplayAbilityActorInfo* ActorInfo);
};
