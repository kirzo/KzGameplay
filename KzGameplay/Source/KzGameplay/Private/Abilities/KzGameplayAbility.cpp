// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"

UKzGameplayAbility::UKzGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

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

void UKzGameplayAbility::PostLoad()
{
	Super::PostLoad();

	// Assets authored before InputBindings carried a single pair; fold it in so they keep working
	if (InputPolicy != EKzAbilityInputPolicy::None && InputTag.IsValid() && InputBindings.IsEmpty())
	{
		FKzAbilityInput Migrated;
		Migrated.InputTag = InputTag;
		Migrated.Policy = InputPolicy;

		InputBindings.Add(Migrated);
	}
}

EKzAbilityInputPolicy UKzGameplayAbility::GetInputPolicy(FGameplayTag QueryTag) const
{
	for (const FKzAbilityInput& Binding : InputBindings)
	{
		if (Binding.InputTag == QueryTag)
		{
			return Binding.Policy;
		}
	}

	return EKzAbilityInputPolicy::None;
}

FGameplayTagContainer UKzGameplayAbility::GetBoundInputTags() const
{
	FGameplayTagContainer Tags;
	for (const FKzAbilityInput& Binding : InputBindings)
	{
		Tags.AddTag(Binding.InputTag);
	}

	return Tags;
}

bool UKzGameplayAbility::IsInputPressed(FGameplayTag QueryTag) const
{
	const UKzAbilitySystemComponent* ASC = Cast<UKzAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC && ASC->IsInputTagPressed(QueryTag);
}

FVector UKzGameplayAbility::GetMoveInput() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const UKzInputHandlerComponent* InputHandler = Avatar ? Avatar->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	const FVector Intended = InputHandler ? InputHandler->GetIntendedInput(KzTags::Input::Move) : FVector::ZeroVector;
	if (!Intended.IsNearlyZero())
	{
		return Intended;
	}

	const APawn* Pawn = Cast<APawn>(Avatar);
	return Pawn ? Pawn->GetLastMovementInputVector() : FVector::ZeroVector;
}