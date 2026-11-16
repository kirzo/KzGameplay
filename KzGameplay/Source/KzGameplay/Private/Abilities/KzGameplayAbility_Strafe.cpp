// Copyright 2026 kirzo

#include "Abilities/KzGameplayAbility_Strafe.h"
#include "Characters/KzCharacterTags.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "InputTriggers.h"

UKzGameplayAbility_Strafe::UKzGameplayAbility_Strafe()
{
	// The control rotation and the facing are the owning client's, which the server hears through the movement
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FKzAbilityInput Binding;
	Binding.InputTag = KzTags::Input::Strafe;
	Binding.Policy = EKzAbilityInputPolicy::ActivateAndListen;
	InputBindings.Add(Binding);

	ActivationOwnedTags.AddTag(KzTags::Character::State_Strafing);
}

void UKzGameplayAbility_Strafe::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	InputHandler = Avatar ? Avatar->FindComponentByClass<UKzInputHandlerComponent>() : nullptr;
	if (UKzInputHandlerComponent* Handler = InputHandler.Get())
	{
		Handler->OnInputAxis.AddDynamic(this, &ThisClass::HandleInputAxis);
	}
}

void UKzGameplayAbility_Strafe::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UKzGameplayAbility_Strafe::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UKzInputHandlerComponent* Handler = InputHandler.Get())
	{
		Handler->OnInputAxis.RemoveDynamic(this, &ThisClass::HandleInputAxis);
	}
	InputHandler.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UKzGameplayAbility_Strafe::HandleInputAxis(FGameplayTag AxisTag, const FInputActionValue& Value, ETriggerEvent TriggerEvent)
{
	// Zero while ignored or once let go, which keeps the last direction
	const FVector Direction = Value.Get<FVector>();
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (TriggerEvent == ETriggerEvent::Triggered && Controller && !Direction.IsNearlyZero() && GetInputPolicy(AxisTag) != EKzAbilityInputPolicy::None)
	{
		Controller->SetControlRotation(Direction.Rotation());
	}
}
