// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "Interaction/KzInteractionTypes.h"
#include "KzGameplayAbility_Interaction.generated.h"

/**
 * An ability that lasts exactly as long as the interaction that started it: do something, wait, undo it.
 *
 * Most interaction abilities are that shape and nothing more, so this one takes both halves as data and
 * leaves the graph empty. What a subclass really contributes is its tags: ActivationOwnedTags to say what
 * the avatar is now doing, ActivationBlockedTags to say when it cannot start. That is GAS being used for
 * what it is good at instead of being reimplemented elsewhere.
 *
 * Effects on the avatar's presentation (focus, camera) belong here. Effects on the object's own state
 * belong on the interaction, registered next to the code that applies them.
 */
UCLASS(Abstract)
class KZGAMEPLAY_API UKzGameplayAbility_Interaction : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_Interaction();

protected:
	/** Runs as the ability starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	FScriptableAction OnBeginAction;

	/** Runs as it ends, however it ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	FScriptableAction OnEndAction;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** The payload we activated with, kept so the ending action sees the same context as the beginning one. */
	UPROPERTY()
	FGameplayEventData ActivationPayload;

	/**
	 * The interaction we were driving when we started. Ending takes it with us: an ability can die for
	 * reasons the interaction knows nothing about (another ability cancelling it, a blocking tag, a stun),
	 * and an interaction nobody is driving is the same bug as an ability outliving its interaction.
	 *
	 * Safe to end blindly because handles are never reused: if it already ended, or the interactor has
	 * since started another one, this names something that no longer exists and nothing happens.
	 */
	FKzInteractionHandle DrivenInteraction;

	UFUNCTION()
	void OnInteractionEnded(class UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason);

	void FillContext(FScriptableAction& Action) const;
};
