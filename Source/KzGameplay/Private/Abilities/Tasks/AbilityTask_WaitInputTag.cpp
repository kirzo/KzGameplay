// Copyright 2026 kirzo

#include "Abilities/Tasks/AbilityTask_WaitInputTag.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "Abilities/KzGameplayAbility.h"

UAbilityTask_WaitInputTag::UAbilityTask_WaitInputTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = false;
}

UAbilityTask_WaitInputTag* UAbilityTask_WaitInputTag::WaitInputTag(UGameplayAbility* OwningAbility, FGameplayTagContainer InputTags, bool bTriggerOnce)
{
	UAbilityTask_WaitInputTag* Task = NewAbilityTask<UAbilityTask_WaitInputTag>(OwningAbility);
	Task->WatchedTags = InputTags;
	Task->bEndOnFirst = bTriggerOnce;

	return Task;
}

UKzAbilitySystemComponent* UAbilityTask_WaitInputTag::GetKzAbilitySystem() const
{
	return Cast<UKzAbilitySystemComponent>(AbilitySystemComponent.Get());
}

void UAbilityTask_WaitInputTag::Activate()
{
	Super::Activate();

	// An empty list means the ability's own bindings, so a task usually needs no configuration at all
	if (WatchedTags.IsEmpty())
	{
		if (const UKzGameplayAbility* KzAbility = Cast<UKzGameplayAbility>(Ability))
		{
			WatchedTags = KzAbility->GetBoundInputTags();
		}
	}

	if (UKzAbilitySystemComponent* ASC = GetKzAbilitySystem())
	{
		ASC->OnAbilityInputTag.AddDynamic(this, &UAbilityTask_WaitInputTag::HandleInputTag);
	}
}

void UAbilityTask_WaitInputTag::HandleInputTag(FGameplayTag InputTag, bool bPressed)
{
	if (!WatchedTags.HasTagExact(InputTag))
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bPressed)
		{
			OnPressed.Broadcast(InputTag);
		}
		else
		{
			OnReleased.Broadcast(InputTag);
		}
	}

	if (bEndOnFirst)
	{
		EndTask();
	}
}

void UAbilityTask_WaitInputTag::OnDestroy(bool AbilityIsEnding)
{
	if (UKzAbilitySystemComponent* ASC = GetKzAbilitySystem())
	{
		ASC->OnAbilityInputTag.RemoveDynamic(this, &UAbilityTask_WaitInputTag::HandleInputTag);
	}

	Super::OnDestroy(AbilityIsEnding);
}
