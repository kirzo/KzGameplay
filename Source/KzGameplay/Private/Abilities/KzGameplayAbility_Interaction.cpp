// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_Interaction.h"
#include "Abilities/Tasks/AbilityTask_WaitInteractionEnded.h"
#include "Interaction/KzInteractorComponent.h"
#include "GameFramework/Actor.h"

UKzGameplayAbility_Interaction::UKzGameplayAbility_Interaction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Everything either half could want, without a cast: the avatar, and the payload it arrived with
	OnBeginAction.AddContextProperty<AActor*>(TEXT("Avatar"));
	OnBeginAction.AddContextProperty<FGameplayEventData>(TEXT("EventData"));

	OnEndAction.AddContextProperty<AActor*>(TEXT("Avatar"));
	OnEndAction.AddContextProperty<FGameplayEventData>(TEXT("EventData"));
}

void UKzGameplayAbility_Interaction::FillContext(FScriptableAction& Action) const
{
	Action.ResetContext();
	Action.SetContextProperty(TEXT("Avatar"), GetAvatarActorFromActorInfo());
	Action.SetContextProperty(TEXT("EventData"), ActivationPayload);
}

void UKzGameplayAbility_Interaction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData)
	{
		ActivationPayload = *TriggerEventData;
	}

	FillContext(OnBeginAction);
	OnBeginAction.Run(this);

	// Straight from the subsystem rather than through whatever ability happens to announce the end
	UAbilityTask_WaitInteractionEnded* WaitEnd = UAbilityTask_WaitInteractionEnded::WaitInteractionEnded(this, nullptr);
	WaitEnd->OnInteractionEnded.AddDynamic(this, &UKzGameplayAbility_Interaction::OnInteractionEnded);
	WaitEnd->ReadyForActivation();
}

void UKzGameplayAbility_Interaction::OnInteractionEnded(UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason)
{
	K2_EndAbility();
}

void UKzGameplayAbility_Interaction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Before Super, which is what tears the ability down and takes its tasks with it
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		FillContext(OnEndAction);
		OnEndAction.Run(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
