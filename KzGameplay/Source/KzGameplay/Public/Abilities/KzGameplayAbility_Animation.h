// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "KzGameplayAbility_Animation.generated.h"

class FKzAnimationPlay;
class UKzAnimationNotifyState;
class UKzAnimationSource;
class UKzInputHandlerComponent;

/**
 * Plays an animation and ends with it, such as a taunt or a dodge: with Montage (By Query), one of those the avatar's database tags for it. A hit
 * reaction that takes over the slot cuts it short. Its Activation Owned Tags say what it is doing while it plays, and the animation's windows the
 * rest: a Move Window to travel, a Collision Window to pass through enemies, a Cancel Window for Cancelable By to cut it.
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

	/** Turns to the direction pressed as it starts, such as a dodge. Off, or with nothing pressed, it keeps the way it faces. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	bool bFaceInput = false;

	/** Abilities that may cut it only inside its animation's Cancel Windows, such as attacking at the end of a dodge. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FGameplayTagContainer CancelableBy;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void OnAnimationNotify(const UKzAnimationNotifyState* Notify, bool bBegin, float Duration);
	void OnAnimationBlendOut();
	void OnAnimationEnded(bool bInterrupted);

	TSharedPtr<FKzAnimationPlay> Play;

	/** Whose movement input it ignores, to give it back. */
	TWeakObjectPtr<UKzInputHandlerComponent> IgnoredInput;

	/** Cancel Windows open to abilities. */
	int32 CancelWindows = 0;
};