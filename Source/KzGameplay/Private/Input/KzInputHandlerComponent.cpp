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
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

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

	// The base layer is the character's own: rebinding it drops everything an item had pushed, which is
	// correct, since a possession change means those items are no longer in these hands
	for (int32 Index = ActiveProfiles.Num() - 1; Index >= 0; --Index)
	{
		UnbindProfileLayer(Pawn, ActiveProfiles[Index]);
	}
	ActiveProfiles.Reset();

	UKzInputProfile* BaseProfile = ProfileToUse ? ProfileToUse : DefaultInputProfile.Get();
	if (!BaseProfile)
	{
		return;
	}

	BindProfileLayer(Pawn, BaseProfile);
}

void UKzInputHandlerComponent::PushInputProfile(UKzInputProfile* Profile)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Profile || !Pawn || ActiveProfiles.ContainsByPredicate([Profile](const FActiveProfile& Layer) { return Layer.Profile == Profile; }))
	{
		return;
	}

	BindProfileLayer(Pawn, Profile);
}

void UKzInputHandlerComponent::RemoveInputProfile(UKzInputProfile* Profile)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	const int32 Index = ActiveProfiles.IndexOfByPredicate([Profile](const FActiveProfile& Layer) { return Layer.Profile == Profile; });
	if (Index == INDEX_NONE)
	{
		return;
	}

	UnbindProfileLayer(Pawn, ActiveProfiles[Index]);
	ActiveProfiles.RemoveAt(Index);
}

const FKzInputAction* UKzInputHandlerComponent::FindActionConfig(const FGameplayTag& InputTag) const
{
	// Topmost first, so a profile pushed later retunes a tag the one below already declared
	for (int32 Index = ActiveProfiles.Num() - 1; Index >= 0; --Index)
	{
		if (const UKzInputProfile* Profile = ActiveProfiles[Index].Profile)
		{
			if (const FKzInputAction* Found = Profile->FindActionConfigForTag(InputTag))
			{
				return Found;
			}
		}
	}

	return nullptr;
}

void UKzInputHandlerComponent::BindProfileLayer(APawn* Pawn, UKzInputProfile* Profile)
{
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	FActiveProfile Layer;
	Layer.Profile = Profile;

	for (const FKzInputAction& Action : Profile->InputActions)
	{
		if (!Action.InputAction || !Action.InputTag.IsValid())
		{
			continue;
		}

		if (Action.Routing == EKzInputRouting::Ability)
		{
			Layer.BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Started, this, &UKzInputHandlerComponent::Input_ActionPressed, Action.InputTag, Action.OnStartedEvent).GetHandle());
			Layer.BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &UKzInputHandlerComponent::Input_ActionReleased, Action.InputTag, Action.OnCompletedEvent).GetHandle());
			Layer.BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Canceled, this, &UKzInputHandlerComponent::Input_ActionReleased, Action.InputTag, Action.OnCompletedEvent).GetHandle());
		}
		else
		{
			// Broadcast across the analog lifecycle so consumers can detect begin (Started),
			// per-frame updates (Triggered), and end (Completed/Canceled) via the delegate's phase.
			for (ETriggerEvent Phase : { ETriggerEvent::Started, ETriggerEvent::Triggered, ETriggerEvent::Completed, ETriggerEvent::Canceled })
			{
				Layer.BindHandles.Add(EnhancedInput->BindAction(Action.InputAction, Phase, this, &UKzInputHandlerComponent::Input_Axis, Action.InputTag, Phase).GetHandle());
			}
		}
	}

	// The keys travel with the profile, so nobody has to remember to keep a second asset in step
	if (Profile->MappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayerInput(Pawn))
		{
			Subsystem->AddMappingContext(Profile->MappingContext, Profile->ContextPriority);
			Layer.bAppliedContext = true;
		}
	}

	ActiveProfiles.Add(MoveTemp(Layer));
}

void UKzInputHandlerComponent::UnbindProfileLayer(APawn* Pawn, FActiveProfile& Layer)
{
	if (Pawn)
	{
		if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
		{
			for (uint32 Handle : Layer.BindHandles)
			{
				EnhancedInput->RemoveBindingByHandle(Handle);
			}
		}

		if (Layer.bAppliedContext && Layer.Profile && Layer.Profile->MappingContext)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayerInput(Pawn))
			{
				Subsystem->RemoveMappingContext(Layer.Profile->MappingContext);
			}
		}
	}

	Layer.BindHandles.Reset();
	Layer.bAppliedContext = false;
}

UEnhancedInputLocalPlayerSubsystem* UKzInputHandlerComponent::GetLocalPlayerInput(const APawn* Pawn) const
{
	// AI pawns have no local player, and therefore no keys: their bindings simply never fire
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
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
	const FKzInputAction* ActionConfig = FindActionConfig(InputTag);
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
		if (const FKzInputAction* ActionConfig = FindActionConfig(InputTag))
		{
			EventTagToRelease = ActionConfig->OnCompletedEvent;
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