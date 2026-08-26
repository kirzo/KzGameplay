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

	for (const auto& [InputTag, Modifiers] : DefaultModifiers)
	{
		for (UKzInputModifier* Modifier : Modifiers)
		{
			if (Modifier)
			{
				// Defaults live on the component template, so a per-owner modifier has to be copied
				// or every character of this class would be sharing one
				PushInputModifier(InputTag, ResolveModifierInstance(Modifier));
			}
		}
	}

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

	ActiveInputProfile = ProfileToUse ? ProfileToUse : DefaultInputProfile.Get();
	if (!ActiveInputProfile)
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
	for (const FKzInputAction& Action : ActiveInputProfile->InputActions)
	{
		if (!Action.InputAction || !Action.InputTag.IsValid())
		{
			continue;
		}

		if (Action.Routing == EKzInputRouting::Ability)
		{
			BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Started, this, &UKzInputHandlerComponent::Input_ActionPressed, Action.InputTag, Action.OnStartedEvent).GetHandle());
			BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &UKzInputHandlerComponent::Input_ActionReleased, Action.InputTag, Action.OnCompletedEvent).GetHandle());
			BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Canceled, this, &UKzInputHandlerComponent::Input_ActionReleased, Action.InputTag, Action.OnCompletedEvent).GetHandle());
		}
		else
		{
			// Broadcast across the analog lifecycle so consumers can detect begin (Started),
			// per-frame updates (Triggered), and end (Completed/Canceled) via the delegate's phase.
			for (ETriggerEvent Phase : { ETriggerEvent::Started, ETriggerEvent::Triggered, ETriggerEvent::Completed, ETriggerEvent::Canceled })
			{
				BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, Phase, this, &UKzInputHandlerComponent::Input_Axis, Action.InputTag, Phase).GetHandle());
			}
		}
	}
}

void UKzInputHandlerComponent::Input_ActionPressed(FGameplayTag InputTag, FGameplayTag EventTag)
{
	if (IsInputIgnored(InputTag)) return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (UKzAbilitySystemComponent* KzASC = Cast<UKzAbilitySystemComponent>(ASC))
		{
			KzASC->AbilityInputTagPressed(InputTag);
		}

		if (EventTag.IsValid())
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetOwner();
			Payload.Target = GetOwner();
			Payload.EventTag = EventTag;
			ASC->HandleGameplayEvent(EventTag, &Payload);
		}
	}
}

void UKzInputHandlerComponent::Input_ActionReleased(FGameplayTag InputTag, FGameplayTag EventTag)
{
	if (IsInputIgnored(InputTag)) return;

	ExecuteActionReleased(InputTag, EventTag);
}

void UKzInputHandlerComponent::ExecuteActionReleased(FGameplayTag InputTag, FGameplayTag EventTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (UKzAbilitySystemComponent* KzASC = Cast<UKzAbilitySystemComponent>(ASC))
		{
			KzASC->AbilityInputTagReleased(InputTag);
		}

		if (EventTag.IsValid())
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetOwner();
			Payload.Target = GetOwner();
			Payload.EventTag = EventTag;
			ASC->HandleGameplayEvent(EventTag, &Payload);
		}
	}
}

void UKzInputHandlerComponent::Input_Axis(const FInputActionValue& Value, FGameplayTag InputTag, ETriggerEvent TriggerEvent)
{
	FVector RawVector = Value.Get<FVector>();
	FVector ModifiedVector = ProcessInput(InputTag, RawVector); // This checks ignores

	RawInputs.Add(InputTag, RawVector);
	ProcessedInputs.Add(InputTag, ModifiedVector);

	OnInputAxis.Broadcast(InputTag, FInputActionValue(ModifiedVector), TriggerEvent);

	// An analog action announces its start and end like any other. Without this the profile's
	// OnStartedEvent is authored, saved, and never sent: only the digital path was wiring them up
	const FKzInputAction* ActionConfig = ActiveInputProfile ? ActiveInputProfile->FindActionConfigForTag(InputTag) : nullptr;
	if (!ActionConfig)
	{
		return;
	}

	if (TriggerEvent == ETriggerEvent::Started)
	{
		Input_ActionPressed(InputTag, ActionConfig->OnStartedEvent);
	}
	else if (TriggerEvent == ETriggerEvent::Completed || TriggerEvent == ETriggerEvent::Canceled)
	{
		Input_ActionReleased(InputTag, ActionConfig->OnCompletedEvent);
	}
}

void UKzInputHandlerComponent::PushInputIgnore(FGameplayTag InputTag, FName SourceID, bool bIgnoreInput, int32 Priority)
{
	if (!InputTag.IsValid()) return;

	bool bWasIgnored = IsInputIgnored(InputTag);
	IgnoreInputStacks.FindOrAdd(InputTag).Push(bIgnoreInput, SourceID, Priority);
	bool bIsNowIgnored = IsInputIgnored(InputTag);

	if (!bWasIgnored && bIsNowIgnored)
	{
		// Find the configured Completed event to properly tell GAS that the action ended
		FGameplayTag EventTagToRelease;
		if (ActiveInputProfile)
		{
			if (const FKzInputAction* ActionConfig = ActiveInputProfile->FindActionConfigForTag(InputTag))
			{
				EventTagToRelease = ActionConfig->OnCompletedEvent;
			}
		}

		// Force the release
		ExecuteActionReleased(InputTag, EventTagToRelease);

		// Zero out the analog axes
		OnInputAxis.Broadcast(InputTag, FInputActionValue(), ETriggerEvent::Canceled);
	}
}

void UKzInputHandlerComponent::RemoveInputIgnore(FGameplayTag InputTag, FName SourceID)
{
	if (auto Stack = IgnoreInputStacks.Find(InputTag))
	{
		Stack->Remove(SourceID);
		if (Stack->IsEmpty())
		{
			IgnoreInputStacks.Remove(InputTag);
		}
	}
}

bool UKzInputHandlerComponent::IsInputIgnored(FGameplayTag InputTag) const
{
	if (auto Stack = IgnoreInputStacks.Find(InputTag))
	{
		return !Stack->IsEmpty() && Stack->Top();
	}
	return false;
}

void UKzInputHandlerComponent::PushInputModifier(FGameplayTag InputTag, UKzInputModifier* Modifier)
{
	if (InputTag.IsValid() && Modifier)
	{
		ModifierStacks.FindOrAdd(InputTag).Push(Modifier);
	}
}

void UKzInputHandlerComponent::RemoveInputModifier(FGameplayTag InputTag, UKzInputModifier* Modifier)
{
	if (FKzInputModifierStack* Stack = ModifierStacks.Find(InputTag))
	{
		Stack->Remove(Modifier);
		if (Stack->IsEmpty())
		{
			ModifierStacks.Remove(InputTag);
		}
	}
}

UKzInputModifier* UKzInputHandlerComponent::ResolveModifierInstance(UKzInputModifier* Modifier)
{
	if (!Modifier || Modifier->InstancingPolicy == EKzInputModifierInstancing::Shared)
	{
		return Modifier;
	}

	return DuplicateObject<UKzInputModifier>(Modifier, this);
}

UKzInputModifier* UKzInputHandlerComponent::PushInputModifierOfClass(FGameplayTag InputTag, TSubclassOf<UKzInputModifier> ModifierClass)
{
	if (!InputTag.IsValid() || !ModifierClass)
	{
		return nullptr;
	}

	UKzInputModifier* Instance = ModifierClass->GetDefaultObject<UKzInputModifier>();
	if (Instance->InstancingPolicy == EKzInputModifierInstancing::PerOwner)
	{
		Instance = NewObject<UKzInputModifier>(this, ModifierClass);
	}

	PushInputModifier(InputTag, Instance);
	return Instance;
}

FVector UKzInputHandlerComponent::GetRawInput(FGameplayTag InputTag) const
{
	const FVector* Found = RawInputs.Find(InputTag);
	return Found ? *Found : FVector::ZeroVector;
}

FVector UKzInputHandlerComponent::GetProcessedInput(FGameplayTag InputTag) const
{
	const FVector* Found = ProcessedInputs.Find(InputTag);
	return Found ? *Found : FVector::ZeroVector;
}

FVector UKzInputHandlerComponent::ProcessInput(FGameplayTag InputTag, const FVector& RawInput)
{
	if (IsInputIgnored(InputTag))
	{
		return FVector::ZeroVector;
	}

	if (FKzInputModifierStack* Stack = ModifierStacks.Find(InputTag))
	{
		return Stack->Process(GetOwner(), RawInput);
	}

	return RawInput;
}