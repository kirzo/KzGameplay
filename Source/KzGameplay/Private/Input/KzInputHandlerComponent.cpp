// Copyright 2026 kirzo

#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputProfile.h"
#include "EnhancedInputComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

UE_DISABLE_OPTIMIZATION

UKzInputHandlerComponent::UKzInputHandlerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKzInputHandlerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		// Check if the InputComponent is already valid (e.g., late BeginPlay)
		if (PawnOwner->InputComponent)
		{
			TryBindInput(PawnOwner);
		}

		// Always subscribe to Restarted in case the player is unpossessed and repossessed later
		PawnOwner->ReceiveRestartedDelegate.AddDynamic(this, &UKzInputHandlerComponent::OnPawnRestarted);
	}
}

void UKzInputHandlerComponent::OnPawnRestarted(APawn* Pawn)
{
	TryBindInput(Pawn);
}

void UKzInputHandlerComponent::InitializeInput(UKzInputProfile* OverrideProfile)
{
	TryBindInput(Cast<APawn>(GetOwner()), OverrideProfile);
}

void UKzInputHandlerComponent::TryBindInput(APawn* Pawn, UKzInputProfile* ProfileToUse)
{
	if (!Pawn)
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	UKzInputProfile* ActiveProfile = ProfileToUse ? ProfileToUse : DefaultInputProfile.Get();
	if (!ActiveProfile)
	{
		return;
	}

	// 1. Unbind previous actions to prevent ghost triggers
	for (uint32 Handle : BindHandles)
	{
		EnhancedInput->RemoveBindingByHandle(Handle);
	}
	BindHandles.Empty();

	// 2. Bind new actions and store their handles
	for (const FKzInputAction& Action : ActiveProfile->InputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			// Bind Pressed (Started)
			FEnhancedInputActionEventBinding& PressedBind = EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Started, this, &UKzInputHandlerComponent::Input_ActionPressed, Action.InputTag);
			BindHandles.Add(PressedBind.GetHandle());

			// Bind Released (Completed)
			FEnhancedInputActionEventBinding& ReleasedBind = EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &UKzInputHandlerComponent::Input_ActionReleased, Action.InputTag);
			BindHandles.Add(ReleasedBind.GetHandle());
		}
	}
}


void UKzInputHandlerComponent::Input_ActionPressed(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetOwner();
		Payload.Target = GetOwner();
		Payload.EventTag = InputTag;

		ASC->HandleGameplayEvent(InputTag, &Payload);

		// Note for 'Wait Input Press' nodes:
		// If you are using a custom ASC (like Lyra), this is where you would call your custom
		// ASC->AbilityInputTagPressed(InputTag) function to update the internal AbilitySpecs.
	}
}

void UKzInputHandlerComponent::Input_ActionReleased(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		// Note for 'Wait Input Release' nodes:
		// Call your custom ASC->AbilityInputTagReleased(InputTag) here if applicable.
	}
}

UE_ENABLE_OPTIMIZATION