// Copyright 2026 kirzo

#include "Abilities/Tasks/AbilityTask_WaitInteractionEnded.h"
#include "Interaction/KzInteractorComponent.h"
#include "GameFramework/Actor.h"

UAbilityTask_WaitInteractionEnded::UAbilityTask_WaitInteractionEnded(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = false;
}

UAbilityTask_WaitInteractionEnded* UAbilityTask_WaitInteractionEnded::WaitInteractionEnded(UGameplayAbility* OwningAbility, UKzInteractorComponent* Interactor)
{
	UAbilityTask_WaitInteractionEnded* Task = NewAbilityTask<UAbilityTask_WaitInteractionEnded>(OwningAbility);
	Task->Interactor = Interactor;

	return Task;
}

void UAbilityTask_WaitInteractionEnded::Activate()
{
	Super::Activate();

	if (!Interactor)
	{
		AActor* Avatar = GetAvatarActor();
		Interactor = Avatar ? Avatar->FindComponentByClass<UKzInteractorComponent>() : nullptr;
	}

	// Nothing to wait for: firing right away beats leaving the ability hanging on an interaction that
	// ended before we got here, or never started
	if (!Interactor || !Interactor->IsInteractingContinuously())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInteractionEnded.Broadcast(nullptr, EKzInteractionEndReason::Interrupted);
		}

		EndTask();
		return;
	}

	Interactor->OnInteractionEnded.AddDynamic(this, &UAbilityTask_WaitInteractionEnded::HandleInteractionEnded);
}

void UAbilityTask_WaitInteractionEnded::HandleInteractionEnded(UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnInteractionEnded.Broadcast(Interactable, Reason);
	}

	EndTask();
}

void UAbilityTask_WaitInteractionEnded::OnDestroy(bool AbilityIsEnding)
{
	if (Interactor)
	{
		Interactor->OnInteractionEnded.RemoveDynamic(this, &UAbilityTask_WaitInteractionEnded::HandleInteractionEnded);
	}

	Super::OnDestroy(AbilityIsEnding);
}
