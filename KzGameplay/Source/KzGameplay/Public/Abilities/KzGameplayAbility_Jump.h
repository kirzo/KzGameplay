// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "KzGameplayAbility_Jump.generated.h"

/**
 * Jumps the avatar character on Input.Jump, as the engine's Character Jump ability does: Kz input reaches only Kz abilities that bind its tag,
 * so that one never hears it. Held, the jump goes on for the character's Jump Max Hold Time; let go, it stops. With Jump Max Count above 1 it
 * jumps again in the air. Granted by Capability.Jump.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_Jump : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_Jump();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
