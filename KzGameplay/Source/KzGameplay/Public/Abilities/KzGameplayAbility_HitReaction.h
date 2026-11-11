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
 * attack tags (Attack.GuardBreak…) and the side it came from (Direction.Front, Back, Left or Right) in its context. A new hit restarts it;
 * State.SuperArmor and State.Dead block it.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_HitReaction : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_HitReaction();

protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Reaction")
	TObjectPtr<UKzAnimationSource> Animation;

	/** Applied by reaction, such as a stun for Reaction.Parried; a reaction without an entry takes its closest parent's. */
	UPROPERTY(EditDefaultsOnly, Category = "Reaction", meta = (Categories = "Reaction", ForceInlineRow))
	TMap<FGameplayTag, TSubclassOf<UGameplayEffect>> Effects;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void OnAnimationEnded(bool bInterrupted);

	TSharedPtr<FKzAnimationPlay> Play;
};