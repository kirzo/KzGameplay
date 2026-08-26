// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_InteractionAction.h"
#include "Abilities/Tasks/AbilityTask_WaitInputTag.h"
#include "Abilities/Tasks/AbilityTask_WaitInteractionEnded.h"
#include "Interaction/KzInteractorComponent.h"
#include "Interaction/KzInteractableComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Actor.h"

UKzGameplayAbility_InteractionAction::UKzGameplayAbility_InteractionAction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

UAnimMontage* UKzGameplayAbility_InteractionAction::ResolveMontage_Implementation(FGameplayTag AnimationTag) const
{
	return nullptr;
}

UKzInteractorComponent* UKzGameplayAbility_InteractionAction::GetInteractor() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar ? Avatar->FindComponentByClass<UKzInteractorComponent>() : nullptr;
}

void UKzGameplayAbility_InteractionAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UKzInteractorComponent* Interactor = GetInteractor();
	if (!Interactor || !Interactor->IsInteractingContinuously())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_WaitInputTag* WaitInput = UAbilityTask_WaitInputTag::WaitInputTag(this, FGameplayTagContainer(), false);
	WaitInput->OnPressed.AddDynamic(this, &UKzGameplayAbility_InteractionAction::OnInputPressed);
	WaitInput->ReadyForActivation();

	// The interaction outliving this ability would leave the actions armed with nothing behind them
	UAbilityTask_WaitInteractionEnded* WaitEnd = UAbilityTask_WaitInteractionEnded::WaitInteractionEnded(this, Interactor);
	WaitEnd->OnInteractionEnded.AddDynamic(this, &UKzGameplayAbility_InteractionAction::OnInteractionEnded);
	WaitEnd->ReadyForActivation();
}

void UKzGameplayAbility_InteractionAction::OnInputPressed(FGameplayTag InputTag)
{
	// One action at a time: a second press mid-animation would stack montages over each other
	if (RunningAction.IsValid())
	{
		return;
	}

	UKzInteractorComponent* Interactor = GetInteractor();
	UKzInteractableComponent* Interactable = Interactor ? Interactor->GetActiveInteractable() : nullptr;
	if (!Interactable || !Interactable->CanRunAction(InputTag, Interactor))
	{
		return;
	}

	const FKzInteractionAction* Action = Interactable->FindAction(InputTag);
	if (!Action)
	{
		return;
	}

	RunningAction = InputTag;

	UAnimMontage* Montage = Action->AnimationTag.IsValid() ? ResolveMontage(Action->AnimationTag) : nullptr;
	if (!Montage)
	{
		// No animation to wait for, so the effect is the press itself
		CommitRunningAction();
		RunningAction = FGameplayTag();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	PlayTask->OnCompleted.AddDynamic(this, &UKzGameplayAbility_InteractionAction::OnMontageFinished);
	PlayTask->OnInterrupted.AddDynamic(this, &UKzGameplayAbility_InteractionAction::OnMontageFinished);
	PlayTask->OnCancelled.AddDynamic(this, &UKzGameplayAbility_InteractionAction::OnMontageFinished);
	PlayTask->ReadyForActivation();

	if (Action->EffectNotify.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* WaitNotify = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, Action->EffectNotify, nullptr, true);
		WaitNotify->EventReceived.AddDynamic(this, &UKzGameplayAbility_InteractionAction::OnEffectNotify);
		WaitNotify->ReadyForActivation();
	}
	else
	{
		// Without a notify the effect lands as the animation starts, which is better than not landing
		CommitRunningAction();
	}
}

void UKzGameplayAbility_InteractionAction::OnEffectNotify(FGameplayEventData Payload)
{
	CommitRunningAction();
}

void UKzGameplayAbility_InteractionAction::OnMontageFinished()
{
	// Whether or not the effect landed, the action is over and the next press is welcome
	RunningAction = FGameplayTag();
}

void UKzGameplayAbility_InteractionAction::CommitRunningAction()
{
	if (!RunningAction.IsValid())
	{
		return;
	}

	UKzInteractorComponent* Interactor = GetInteractor();
	if (UKzInteractableComponent* Interactable = Interactor ? Interactor->GetActiveInteractable() : nullptr)
	{
		Interactable->RunAction(RunningAction, Interactor);
	}
}

void UKzGameplayAbility_InteractionAction::OnInteractionEnded(UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason)
{
	K2_EndAbility();
}
