// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "KzGameplayAbility_Dodge.generated.h"

class FKzAnimationPlay;
class UCurveFloat;
class UKzAnimationSource;
class UKzInputHandlerComponent;

/**
 * Rolls or dashes the avatar: turns it to the direction pressed, plays its animation in place and moves it with a root motion force, so the
 * animation needs no root motion of its own. Movement input is ignored meanwhile, so it neither steers nor turns. Ends as the animation blends
 * out, or with the move without an animation.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_Dodge : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_Dodge();

protected:
	/** Played in place. A Tag Window in it can grant State.Invulnerable for part of the dodge. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Dodge")
	TObjectPtr<UKzAnimationSource> Animation;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = "0", Units = "Centimeters"))
	float Distance = 420.0f;

	/** How long the move takes. The animation can go on after it. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = "0.01", Units = "Seconds"))
	float Duration = 0.6f;

	/** Multiplies the speed over the move, 0 to 1 on X. Averaging 1 keeps Distance; none moves at an even speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<UCurveFloat> SpeedCurve;

	/** Turns to the direction pressed first. Off, or with nothing pressed, it dodges the way it faces. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	bool bFaceInput = true;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void OnAnimationBlendOut();
	void OnAnimationEnded(bool bInterrupted);

	UFUNCTION()
	void OnMoveFinished();

	TSharedPtr<FKzAnimationPlay> Play;

	/** Whose movement input it ignores, to give it back. */
	TWeakObjectPtr<UKzInputHandlerComponent> IgnoredInput;
};
