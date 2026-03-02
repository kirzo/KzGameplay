// Copyright 2026 kirzo

#include "Abilities/KzAbilitySystemComponent.h"
#include "Abilities/KzGameplayAbility.h"

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

	bool bInputConsumed = false;

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (const UKzGameplayAbility* KzAbility = Cast<UKzGameplayAbility>(Spec.Ability))
		{
			// Check if this ability cares about THIS specific input tag
			if (KzAbility->InputPolicy != EKzAbilityInputPolicy::None && KzAbility->InputTag == InputTag)
			{
				const bool bActivatesFromInput = (KzAbility->InputPolicy == EKzAbilityInputPolicy::ActivateAndListen);

				// If it reaches here, it inherently has listening permissions (ListenOnly or ActivateAndListen)
				Spec.InputPressed = true;

				if (Spec.IsActive())
				{
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

					bInputConsumed = true;
				}
				else if (bActivatesFromInput)
				{
					// It's inactive, but it has permission to activate!
					TryActivateAbility(Spec.Handle);
					bInputConsumed = true;
				}
			}
		}
	}

	// Fallback for Abilities listening directly via their 'Ability Triggers'
	if (!bInputConsumed)
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetOwnerActor();
		Payload.Target = GetAvatarActor();
		Payload.EventTag = InputTag;

		HandleGameplayEvent(InputTag, &Payload);
	}
}

void UKzAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid() || !AbilityActorInfo.IsValid()) return;
	if (!AbilityActorInfo->IsLocallyControlled() && !AbilityActorInfo->IsNetAuthority()) return;

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (const UKzGameplayAbility* KzAbility = Cast<UKzGameplayAbility>(Spec.Ability))
		{
			// For release, we only care if it's listening
			if (KzAbility->InputPolicy != EKzAbilityInputPolicy::None && KzAbility->InputTag == InputTag)
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