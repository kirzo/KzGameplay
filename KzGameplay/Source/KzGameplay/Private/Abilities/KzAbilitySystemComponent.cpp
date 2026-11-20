// Copyright 2026 kirzo

#include "Abilities/KzAbilitySystemComponent.h"
#include "Abilities/KzGameplayAbility.h"
#include "Combat/KzTimeDilationSubsystem.h"
#include "Engine/World.h"

namespace
{
	/** No Input Buffer reaches this far back: past it, a kept press is let go, and the component stops ticking for it. */
	constexpr float MaxKeptInputAge = 1.0f;
}

UKzAbilitySystemComponent::UKzAbilitySystemComponent()
{
}

void UKzAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid() || !AbilityActorInfo.IsValid())
	{
		return;
	}

	if (!AbilityActorInfo->IsLocallyControlled() && !AbilityActorInfo->IsNetAuthority())
	{
		return;
	}

	PressedInputTags.Add(InputTag);

	// Announce it before any routing, so tasks hear their input whether or not a spec consumes it
	OnAbilityInputTag.Broadcast(InputTag, true);

	bool bHeard = false;
	{
		ABILITYLIST_SCOPE_LOCK();
		for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
		{
			const UKzGameplayAbility* KzAbility = Cast<UKzGameplayAbility>(Spec.Ability);
			const EKzAbilityInputPolicy Policy = KzAbility ? KzAbility->GetInputPolicy(InputTag) : EKzAbilityInputPolicy::None;
			if (Policy == EKzAbilityInputPolicy::None || Policy == EKzAbilityInputPolicy::TriggerEvent)
			{
				continue;
			}

			// If it reaches here, it inherently has listening permissions (ListenOnly or ActivateAndListen)
			Spec.InputPressed = true;

			if (Spec.IsActive())
			{
				bHeard = true;
				if (Spec.Ability->bReplicateInputDirectly && !IsOwnerActorAuthoritative())
				{
					ServerSetInputPressed(Spec.Handle);
				}

				AbilitySpecInputPressed(Spec);

				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
				const FGameplayAbilityActivationInfo& ActivationInfo = Instances.IsEmpty() ? Spec.ActivationInfo : Instances.Last()->GetCurrentActivationInfoRef();
				PRAGMA_ENABLE_DEPRECATION_WARNINGS

				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, ActivationInfo.GetActivationPredictionKey());
			}
		}
	}

	// A press only blocked abilities wanted waits a moment, in case a Cancel Window opens for them
	bool bTried = false;
	if (!ActivateFromInput(InputTag, bTried) && bTried && !bHeard)
	{
		KeepInput(InputTag);
	}
}

bool UKzAbilitySystemComponent::ActivateFromInput(const FGameplayTag& InputTag, bool& bOutTried)
{
	bOutTried = false;
	bool bActivated = false;
	bool bWantsEvent = false;
	{
		ABILITYLIST_SCOPE_LOCK();
		for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
		{
			const UKzGameplayAbility* KzAbility = Cast<UKzGameplayAbility>(Spec.Ability);
			const EKzAbilityInputPolicy Policy = KzAbility ? KzAbility->GetInputPolicy(InputTag) : EKzAbilityInputPolicy::None;
			if (Policy == EKzAbilityInputPolicy::TriggerEvent)
			{
				bWantsEvent = true;
			}
			else if (Policy == EKzAbilityInputPolicy::ActivateAndListen && !Spec.IsActive())
			{
				bOutTried = true;
				bActivated |= TryActivateAbility(Spec.Handle);
			}
		}
	}

	// Declared, not leftover: an ability asks for the event with TriggerEvent rather than getting it
	// only when nobody else happened to consume the input
	if (bWantsEvent)
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetOwnerActor();
		Payload.Target = GetAvatarActor();
		Payload.EventTag = InputTag;

		bOutTried = true;
		bActivated |= HandleGameplayEvent(InputTag, &Payload) > 0;
	}
	return bActivated;
}

void UKzAbilitySystemComponent::ReplayKeptInput(float Buffer)
{
	const FGameplayTag InputTag = KeptInput;
	const bool bInTime = KeptAge <= Buffer;
	KeepInput(FGameplayTag());

	bool bTried = false;
	if (InputTag.IsValid() && bInTime)
	{
		ActivateFromInput(InputTag, bTried);
	}
}

void UKzAbilitySystemComponent::KeepInput(const FGameplayTag& InputTag)
{
	KeptInput = InputTag;
	KeptAge = 0.0f;
	UpdateShouldTick();
}

bool UKzAbilitySystemComponent::GetShouldTick() const
{
	return Super::GetShouldTick() || KeptInput.IsValid();
}

void UKzAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!KeptInput.IsValid())
	{
		return;
	}

	// In the avatar's own time, so a hit stop holds the buffer; an owner's dilation already comes in DeltaTime
	const AActor* Avatar = GetAvatarActor_Direct();
	KeptAge += DeltaTime * (Avatar && Avatar != GetOwner() ? Avatar->CustomTimeDilation : 1.0f);
	if (KeptAge > MaxKeptInputAge)
	{
		KeepInput(FGameplayTag());
	}
}

void UKzAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	// A new action starts: what was pressed before it is not for its Cancel Windows
	if (KeptInput.IsValid())
	{
		KeepInput(FGameplayTag());
	}

	// Before it plays, so it starts at full speed: such as a dodge out of a hit stop
	UKzTimeDilationSubsystem* TimeDilation = GetWorld() ? GetWorld()->GetSubsystem<UKzTimeDilationSubsystem>() : nullptr;
	if (TimeDilation && Ability)
	{
		TimeDilation->Cancel(GetAvatarActor_Direct(), Ability->GetAssetTags());
	}
}

void UKzAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid() || !AbilityActorInfo.IsValid()) return;
	if (!AbilityActorInfo->IsLocallyControlled() && !AbilityActorInfo->IsNetAuthority()) return;

	PressedInputTags.Remove(InputTag);
	OnAbilityInputTag.Broadcast(InputTag, false);

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (const UKzGameplayAbility* KzAbility = Cast<UKzGameplayAbility>(Spec.Ability))
		{
			const EKzAbilityInputPolicy Policy = KzAbility->GetInputPolicy(InputTag);
			if (Policy != EKzAbilityInputPolicy::None && Policy != EKzAbilityInputPolicy::TriggerEvent)
			{
				Spec.InputPressed = false;

				if (Spec.IsActive())
				{
					if (Spec.Ability->bReplicateInputDirectly && !IsOwnerActorAuthoritative())
					{
						ServerSetInputReleased(Spec.Handle);
					}

					AbilitySpecInputReleased(Spec);

					PRAGMA_DISABLE_DEPRECATION_WARNINGS
					TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
					const FGameplayAbilityActivationInfo& ActivationInfo = Instances.IsEmpty() ? Spec.ActivationInfo : Instances.Last()->GetCurrentActivationInfoRef();
					PRAGMA_ENABLE_DEPRECATION_WARNINGS

					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, ActivationInfo.GetActivationPredictionKey());
				}
			}
		}
	}
}

void UKzAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& Spec)
{
	if (UKzGameplayAbility* Ability = Cast<UKzGameplayAbility>(Spec.Ability))
	{
		ABILITYLIST_SCOPE_LOCK();

		for (const FAbilityTriggerData& TriggerData : Ability->AbilityCancelTriggers)
		{
			FGameplayTag EventTag = TriggerData.TriggerTag;

			if (TriggerData.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent)
			{
				GameplayEventCancelledAbilities.FindOrAdd(EventTag).AddUnique(Spec.Handle);
			}
			else
			{
				OwnedTagCancelledAbilities.FindOrAdd(EventTag).AddUnique(Spec.Handle);

				// Subscribe to the tag change only once per tag
				if (!CancelTagDelegateHandles.Contains(EventTag))
				{
					FDelegateHandle NewHandle = RegisterGameplayTagEvent(EventTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UKzAbilitySystemComponent::CancelTagChanged);
					CancelTagDelegateHandles.Add(EventTag, NewHandle);
				}
			}
		}
	}

	Super::OnGiveAbility(Spec);
}

void UKzAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& Spec)
{
	if (UKzGameplayAbility* Ability = Cast<UKzGameplayAbility>(Spec.Ability))
	{
		ABILITYLIST_SCOPE_LOCK();

		for (const FAbilityTriggerData& TriggerData : Ability->AbilityCancelTriggers)
		{
			FGameplayTag EventTag = TriggerData.TriggerTag;

			if (TriggerData.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent)
			{
				if (TArray<FGameplayAbilitySpecHandle>* Handles = GameplayEventCancelledAbilities.Find(EventTag))
				{
					Handles->Remove(Spec.Handle);
					if (Handles->IsEmpty())
					{
						GameplayEventCancelledAbilities.Remove(EventTag);
					}
				}
			}
			else
			{
				if (TArray<FGameplayAbilitySpecHandle>* Handles = OwnedTagCancelledAbilities.Find(EventTag))
				{
					Handles->Remove(Spec.Handle);
					if (Handles->IsEmpty())
					{
						OwnedTagCancelledAbilities.Remove(EventTag);

						// Clean up the delegate if no more abilities care about this tag
						if (FDelegateHandle* DelHandle = CancelTagDelegateHandles.Find(EventTag))
						{
							UnregisterGameplayTagEvent(*DelHandle, EventTag, EGameplayTagEventType::NewOrRemoved);
							CancelTagDelegateHandles.Remove(EventTag);
						}
					}
				}
			}
		}
	}

	Super::OnRemoveAbility(Spec);
}

int32 UKzAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	const int32 TriggeredCount = Super::HandleGameplayEvent(EventTag, Payload);
	FGameplayTag CurrentTag = EventTag;

	ABILITYLIST_SCOPE_LOCK();

	// Traverse up the tag hierarchy to cancel generic parents (e.g. Event.Damage.Fire triggers Event.Damage cancels)
	while (CurrentTag.IsValid())
	{
		if (TArray<FGameplayAbilitySpecHandle>* CancelledAbilityHandles = GameplayEventCancelledAbilities.Find(CurrentTag))
		{
			for (const FGameplayAbilitySpecHandle& AbilityHandle : *CancelledAbilityHandles)
			{
				CancelAbilityHandle(AbilityHandle);
			}
		}

		CurrentTag = CurrentTag.RequestDirectParent();
	}

	OnGameplayEvent.Broadcast(EventTag, Payload ? *Payload : FGameplayEventData());

	return TriggeredCount;
}

void UKzAbilitySystemComponent::CancelTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		ABILITYLIST_SCOPE_LOCK();

		if (TArray<FGameplayAbilitySpecHandle>* CancelledAbilityHandles = OwnedTagCancelledAbilities.Find(Tag))
		{
			for (const FGameplayAbilitySpecHandle& AbilityHandle : *CancelledAbilityHandles)
			{
				CancelAbilityHandle(AbilityHandle);
			}
		}
	}
}