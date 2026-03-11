// Copyright 2026 kirzo

#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputProfile.h"
#include "EnhancedInputComponent.h"
#include "Abilities/KzGameplayAbility.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

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
		if (UKzAbilitySystemComponent* KzASC = Cast<UKzAbilitySystemComponent>(ASC))
		{
			KzASC->AbilityInputTagPressed(InputTag);
		}
		else
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetOwner();
			Payload.Target = GetOwner();
			Payload.EventTag = InputTag;
			ASC->HandleGameplayEvent(InputTag, &Payload);
		}
	}
}

void UKzInputHandlerComponent::Input_ActionReleased(FGameplayTag InputTag)
{
	if (UKzAbilitySystemComponent* KzASC = Cast<UKzAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner())))
	{
		KzASC->AbilityInputTagReleased(InputTag);
	}
}

void UKzInputHandlerComponent::PushMoveInputIgnore(FName SourceID, bool bIgnoreMoveInput, int32 Priority)
{
	IgnoreMoveInputStack.Push(bIgnoreMoveInput, SourceID, Priority);
	UpdateMoveInputIgnore();
}

void UKzInputHandlerComponent::RemoveMoveInputIgnore(FName SourceID)
{
	if (IgnoreMoveInputStack.Remove(SourceID) > 0)
	{
		UpdateMoveInputIgnore();
	}
}

void UKzInputHandlerComponent::PushLookInputIgnore(FName SourceID, bool bIgnoreLookInput, int32 Priority)
{
	IgnoreLookInputStack.Push(bIgnoreLookInput, SourceID, Priority);
	UpdateLookInputIgnore();
}

void UKzInputHandlerComponent::RemoveLookInputIgnore(FName SourceID)
{
	if (IgnoreLookInputStack.Remove(SourceID) > 0)
	{
		UpdateLookInputIgnore();
	}
}

void UKzInputHandlerComponent::UpdateMoveInputIgnore()
{
	if (APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		if (AController* Controller = PawnOwner->GetController())
		{
			const bool bIgnore = IgnoreMoveInputStack.IsEmpty() ? false : IgnoreMoveInputStack.Top();
			Controller->SetIgnoreMoveInput(bIgnore);
		}
	}
}

void UKzInputHandlerComponent::UpdateLookInputIgnore()
{
	if (APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		if (AController* Controller = PawnOwner->GetController())
		{
			const bool bIgnore = IgnoreLookInputStack.IsEmpty() ? false : IgnoreLookInputStack.Top();
			Controller->SetIgnoreLookInput(bIgnore);
		}
	}
}

void UKzInputHandlerComponent::PushMoveModifier(UKzInputModifier* Modifier)
{
	if (Modifier)
	{
		MoveModifierStack.Push(Modifier);
	}
}

void UKzInputHandlerComponent::RemoveMoveModifier(UKzInputModifier* Modifier)
{
	MoveModifierStack.Remove(Modifier);
}

FVector UKzInputHandlerComponent::ProcessMoveInput(const FVector& RawInput) const
{
	if (!IgnoreMoveInputStack.IsEmpty() && IgnoreMoveInputStack.Top()) return FVector::ZeroVector;
	return MoveModifierStack.Process(GetOwner(), RawInput);
}

void UKzInputHandlerComponent::PushLookModifier(UKzInputModifier* Modifier)
{
	if (Modifier)
	{
		LookModifierStack.Push(Modifier);
	}
}

void UKzInputHandlerComponent::RemoveLookModifier(UKzInputModifier* Modifier)
{
	LookModifierStack.Remove(Modifier);
}

FVector UKzInputHandlerComponent::ProcessLookInput(const FVector& RawInput) const
{
	if (!IgnoreLookInputStack.IsEmpty() && IgnoreLookInputStack.Top()) return FVector::ZeroVector;
	return LookModifierStack.Process(GetOwner(), RawInput);
}