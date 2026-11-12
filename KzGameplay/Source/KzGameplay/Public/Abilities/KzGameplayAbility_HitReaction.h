// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "KzGameplayAbility_HitReaction.generated.h"

class FKzAnimationPlay;
class UGameplayEffect;
class UKzAnimationSource;

/**
 * How the avatar takes a hit, triggered by the Reaction events combo hits send. Its animation, normally By Query, has the reaction, the hit's
 * attack tags (Attack.GuardBreak…) and the side the blow comes in from (one of Directions) in its context. As it starts, the avatar turns for the blow
 * to come in squarely from that side, and a Knockback Window in the animation pushes it the way the blow travels. A new hit restarts it;
 * State.SuperArmor and State.Dead block it.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_HitReaction : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_HitReaction();

	/** The push of the reaction the avatar is playing: which way along the ground, and the hit's Knockback. False when it is not reacting. */
	static bool FindKnockback(const AActor* Avatar, FVector& OutDirection, float& OutScale);

protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Reaction")
	TObjectPtr<UKzAnimationSource> Animation;

	/** Applied by reaction, such as a stun for Reaction.Parried; a reaction without an entry takes its closest parent's. */
	UPROPERTY(EditDefaultsOnly, Category = "Reaction", meta = (Categories = "Reaction", ForceInlineRow))
	TMap<FGameplayTag, TSubclassOf<UGameplayEffect>> Effects;

	/**
	 * The sides a blow can come in from, by the way each faces from the avatar (X forward, Y right, Z up). The closest to where the blow comes from goes
	 * in the animation's context. Without one facing up or down, only where it comes from along the ground counts.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Direction", meta = (Categories = "Direction", ForceInlineRow))
	TMap<FGameplayTag, FVector> Directions;

	/** How far the avatar turns as it starts reacting, for the blow to come in squarely from its side. */
	UPROPERTY(EditDefaultsOnly, Category = "Direction", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float MaxTurn = 45.0f;

	/** How long that turn takes; zero turns at once. */
	UPROPERTY(EditDefaultsOnly, Category = "Direction", meta = (ClampMin = "0", Units = "Seconds"))
	float TurnTime = 0.1f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** The side the blow comes in from, and turns the avatar toward it. */
	FGameplayTag FaceBlow(AActor& Avatar, const FVector& Blow, const FVector& FromInstigator);

	void TickTurn();
	void OnAnimationEnded(bool bInterrupted);

	TSharedPtr<FKzAnimationPlay> Play;

	/** Along the ground, normalized. */
	FVector KnockbackDirection = FVector::ZeroVector;
	float Knockback = 1.0f;

	float TurnFrom = 0.0f;
	float TurnBy = 0.0f;
	float TurnElapsed = 0.0f;
	FTimerHandle TurnTimer;
};