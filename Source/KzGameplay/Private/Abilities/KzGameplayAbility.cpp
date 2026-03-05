// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility.h"
#include "AbilitySystemComponent.h"

UKzGameplayAbility::UKzGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	InputPolicy = EKzAbilityInputPolicy::None;
	bActivateAbilityOnGranted = false;
}

void UKzGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (bActivateAbilityOnGranted && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		// TryActivateAbility handles the execution policy (local vs server) correctly
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}

bool UKzGameplayAbility::IsInputPressed() const
{
	if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Spec->InputPressed;
	}

	return false;
}