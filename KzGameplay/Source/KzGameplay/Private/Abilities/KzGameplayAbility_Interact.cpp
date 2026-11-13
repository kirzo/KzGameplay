// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_Interact.h"
#include "Interaction/KzInteractorComponent.h"
#include "Interaction/KzInteractionTags.h"
#include "GameFramework/Actor.h"

UKzGameplayAbility_Interact::UKzGameplayAbility_Interact()
{
	// Only the machine that scans knows the focus, and the interactor asks the authority for the interaction itself
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	FKzAbilityInput Binding;
	Binding.InputTag = KzTags::Interaction::Input_Interact;
	Binding.Policy = EKzAbilityInputPolicy::ActivateAndListen;
	InputBindings.Add(Binding);
}

bool UKzGameplayAbility_Interact::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UKzInteractorComponent* Interactor = GetInteractor(ActorInfo);
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) && Interactor && Interactor->GetCurrentFocus();
}

void UKzGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UKzInteractorComponent* Interactor = GetInteractor(ActorInfo);
	if (Interactor && CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		Interactor->Interact();
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UKzInteractorComponent* UKzGameplayAbility_Interact::GetInteractor(const FGameplayAbilityActorInfo* ActorInfo)
{
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Avatar ? Avatar->FindComponentByClass<UKzInteractorComponent>() : nullptr;
}
