// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "KzGameplayAbility_Animation.generated.h"

class FKzAnimationPlay;
class UKzAnimationSource;
class UKzInputHandlerComponent;

/**
 * Plays an animation and ends with it, such as a taunt: with Montage (By Query), one of those the avatar's database tags for it. A hit
 * reaction that takes over the slot cuts it short. Its Activation Owned Tags say what it is doing while it plays.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_Animation : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_Animation();

protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Animation")
	TObjectPtr<UKzAnimationSource> Animation;

	/** Ignores movement input while it plays, so the avatar stays put. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	bool bIgnoreMoveInput = true;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void OnAnimationBlendOut();
	void OnAnimationEnded(bool bInterrupted);

	TSharedPtr<FKzAnimationPlay> Play;

	/** Whose movement input it ignores, to give it back. */
	TWeakObjectPtr<UKzInputHandlerComponent> IgnoredInput;
};