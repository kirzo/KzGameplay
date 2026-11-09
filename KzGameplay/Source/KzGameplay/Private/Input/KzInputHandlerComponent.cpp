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
		// Even without input yet, the base profile goes on: injected inputs read its events too
		TryBindInput(PawnOwner);

		// Always subscribe to Restarted in case the player is unpossessed and repossessed later
		PawnOwner->ReceiveRestartedDelegate.AddDynamic(this, &UKzInputHandlerComponent::OnPawnRestarted);
		PawnOwner->ReceiveControllerChangedDelegate.AddDynamic(this, &UKzInputHandlerComponent::OnPawnControllerChanged);
	}
}

void UKzInputHandlerComponent::OnPawnRestarted(APawn* Pawn)
{
	TryBindInput(Pawn);
}

void UKzInputHandlerComponent::OnPawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	// The pawn no longer knows the controller leaving, and its contexts would stay on that player while it drives another pawn
	for (FActiveProfile& Layer : ActiveProfiles)
	{
		UnbindProfileLayer(Pawn, OldController, Layer);
	}
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

	for (int32 Index = ActiveProfiles.Num() - 1; Index >= 0; --Index)
	{
		UnbindProfileLayer(Pawn, Pawn->GetController(), ActiveProfiles[Index]);
	}

	// Only the base is replaced: pushed layers belong to what the pawn holds and can do, which a possession
	// does not change, and the ones pushed before there was input are bound here for the first time
	ActiveProfiles.RemoveAll([](const FActiveProfile& Layer) { return Layer.bIsBase; });

	if (UKzInputProfile* BaseProfile = ProfileToUse ? ProfileToUse : DefaultInputProfile.Get())
	{
		FActiveProfile Base;
		Base.Profile = BaseProfile;
		Base.bIsBase = true;
		ActiveProfiles.Insert(MoveTemp(Base), 0);
	}

	for (FActiveProfile& Layer : ActiveProfiles)
	{
		BindProfileLayer(Pawn, Layer);
	}
}

void UKzInputHandlerComponent::PushInputProfile(UKzInputProfile* Profile)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Profile || !Pawn || ActiveProfiles.ContainsByPredicate([Profile](const FActiveProfile& Layer) { return Layer.Profile == Profile; }))
	{
		return;
	}

	FActiveProfile& Layer = ActiveProfiles.AddDefaulted_GetRef();
	Layer.Profile = Profile;
	BindProfileLayer(Pawn, Layer);
}

void UKzInputHandlerComponent::RemoveInputProfile(UKzInputProfile* Profile)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	const int32 Index = ActiveProfiles.IndexOfByPredicate([Profile](const FActiveProfile& Layer) { return Layer.Profile == Profile; });
	if (Index == INDEX_NONE)
	{
		return;
	}

	UnbindProfileLayer(Pawn, Pawn ? Pawn->GetController() : nullptr, ActiveProfiles[Index]);
	ActiveProfiles.RemoveAt(Index);
}

const FKzInputAction* UKzInputHandlerComponent::FindActionConfig(const FGameplayTag& InputTag) const
{
	const UKzInputProfile* Profile = FindActionProfile(InputTag);
	return Profile ? Profile->FindActionConfigForTag(InputTag) : nullptr;
}

const UKzInputProfile* UKzInputHandlerComponent::FindActionProfile(const FGameplayTag& InputTag) const
{
	// Topmost first, so a profile pushed later retunes a tag the one below already declared
	for (int32 Index = ActiveProfiles.Num() - 1; Index >= 0; --Index)
	{
		const UKzInputProfile* Profile = ActiveProfiles[Index].Profile;
		if (Profile && Profile->FindActionConfigForTag(InputTag))
		{
			return Profile;
		}
	}

	return nullptr;
}

void UKzInputHandlerComponent::BindProfileLayer(APawn* Pawn, FActiveProfile& Layer)
{
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
	UKzInputProfile* Profile = Layer.Profile;
	if (!EnhancedInput || !Profile)
	{
		return;
	}

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
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayerInput(Pawn->GetController()))
		{
			// Derived from the order things were pushed rather than authored: priority only decides who
			// wins a key two contexts both claim, and that answer is already "whatever went on last".
			// A number per asset made unrelated profiles share one space and collide over nothing, and
			// could disagree with the stack that FindActionConfig walks.
			Subsystem->AddMappingContext(Profile->MappingContext, NextContextPriority++);
			Layer.bAppliedContext = true;
		}
	}
}

void UKzInputHandlerComponent::UnbindProfileLayer(APawn* Pawn, const AController* Controller, FActiveProfile& Layer)
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
	}

	if (Layer.bAppliedContext && Layer.Profile && Layer.Profile->MappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayerInput(Controller))
		{
			Subsystem->RemoveMappingContext(Layer.Profile->MappingContext);
		}
	}

	Layer.BindHandles.Reset();
	Layer.bAppliedContext = false;
}

UEnhancedInputLocalPlayerSubsystem* UKzInputHandlerComponent::GetLocalPlayerInput(const AController* Controller) const
{
	// AI controllers have no local player, and therefore no keys: their bindings simply never fire
	const APlayerController* PC = Cast<APlayerController>(Controller);
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
	const FKzInputModifierStack* Stack = ModifierStacks.Find(InputTag);

	RawInputs.Add(InputTag, RawVector);
	ProcessedInputs.Add(InputTag, ModifiedVector);
	IntendedInputs.Add(InputTag, Stack ? Stack->Preview(GetOwner(), RawVector) : RawVector);

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

void UKzInputHandlerComponent::PressInput(FGameplayTag InputTag)
{
	const FKzInputAction* ActionConfig = FindActionConfig(InputTag);
	Input_ActionPressed(InputTag, ActionConfig ? ActionConfig->OnStartedEvent : FGameplayTag());
}

void UKzInputHandlerComponent::ReleaseInput(FGameplayTag InputTag)
{
	const FKzInputAction* ActionConfig = FindActionConfig(InputTag);
	Input_ActionReleased(InputTag, ActionConfig ? ActionConfig->OnCompletedEvent : FGameplayTag());
}

void UKzInputHandlerComponent::SetInputAxis(FGameplayTag InputTag, FVector Value)
{
	if (!Value.IsNearlyZero())
	{
		bool bAlreadyHeld = false;
		HeldAxes.Add(InputTag, &bAlreadyHeld);
		if (!bAlreadyHeld)
		{
			Input_Axis(FInputActionValue(Value), InputTag, ETriggerEvent::Started);
		}
		Input_Axis(FInputActionValue(Value), InputTag, ETriggerEvent::Triggered);
	}
	else if (HeldAxes.Remove(InputTag) > 0)
	{
		Input_Axis(FInputActionValue(FVector::ZeroVector), InputTag, ETriggerEvent::Completed);
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

FVector UKzInputHandlerComponent::GetIntendedInput(FGameplayTag InputTag) const
{
	const FVector* Found = IntendedInputs.Find(InputTag);
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