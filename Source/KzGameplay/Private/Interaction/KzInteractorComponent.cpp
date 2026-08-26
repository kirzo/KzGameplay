// Copyright 2026 kirzo

#include "Interaction/KzInteractorComponent.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractionTags.h"
#include "Scoring/KzTargetScoringLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"

#include "Engine/World.h"
#include "TimerManager.h"

UKzInteractorComponent::UKzInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // We use timers
	ScanRate = 0.1f;

	// Standalone ignores this, so local play pays nothing for it
	SetIsReplicatedByDefault(true);

	// Defaults, not hardcoding: the plugin's own pieces find each other, and a game can still repoint them
	InteractionBegunEventTag = KzTags::Interaction::Begun;
	InteractionEndedEventTag = KzTags::Interaction::Ended;

	FilterRequirement.AddContextProperty<AActor*>(TEXT("Instigator"));
	FilterRequirement.AddContextProperty<UKzInteractorComponent*>(TEXT("Interactor"));
	FilterRequirement.AddContextProperty<UKzInteractableComponent*>(TEXT("Interactable"));
}

void UKzInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	// The end of our interaction comes from the subsystem, never from whoever caused it
	if (UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr)
	{
		Subsystem->OnInteractionBegun.AddDynamic(this, &UKzInteractorComponent::HandleInteractionBegun);
		Subsystem->OnInteractionEnded.AddDynamic(this, &UKzInteractorComponent::HandleInteractionEnded);
	}

	StartScanning();
}

void UKzInteractorComponent::StartScanning()
{
	// Only scan on the local client (or the server if this is an AI).
	if (GetOwner()->GetLocalRole() != ROLE_SimulatedProxy)
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(ScanTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UKzInteractorComponent::PerformScan, ScanRate, true);
		}
	}
}

void UKzInteractorComponent::StopScanning()
{
	if (GetOwner()->GetLocalRole() != ROLE_SimulatedProxy)
	{
		// We use ClearTimer instead of PauseTimer so that when we resume, it evaluates immediately on the first tick
		GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);

		// Clear current focus visually so the UI prompt disappears while we are busy
		if (UKzInteractableComponent* OldInteractable = CurrentFocus.Get())
		{
			OldInteractable->OnEndFocus.Broadcast(this);
			CurrentFocus.Reset();
			OnCurrentInteractableChanged.Broadcast(nullptr, OldInteractable);
		}
	}
}

void UKzInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr)
	{
		Subsystem->OnInteractionBegun.RemoveDynamic(this, &UKzInteractorComponent::HandleInteractionBegun);
		Subsystem->OnInteractionEnded.RemoveDynamic(this, &UKzInteractorComponent::HandleInteractionEnded);

		// Otherwise whatever we were holding waits for an avatar that no longer exists
		if (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld)
		{
			Subsystem->EndInteractionsFor(this, EKzInteractionEndReason::InstigatorLost);
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void UKzInteractorComponent::PerformScan()
{
	UKzInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UKzInteractionSubsystem>();
	if (!Subsystem) return;

#if WITH_GAMEPLAY_DEBUGGER
	LastDebugCandidates.Empty();
#endif

	// Query the grid (broad and narrow phase are done internally)
	FTransform WorldTransform = GetComponentTransform();
	TArray<UKzInteractableComponent*> Candidates = Subsystem->QueryInteractables(Shape, WorldTransform.GetLocation(), WorldTransform.GetRotation());

	UKzInteractableComponent* BestCandidate = nullptr;
	float BestScore = -1.0f;

	const FKzTransformSource AsTransformSource = FKzTransformSource(this);

	for (UKzInteractableComponent* Candidate : Candidates)
	{
#if WITH_GAMEPLAY_DEBUGGER
		FKzInteractionDebugCandidate DebugInfo;
		DebugInfo.Interactable = Candidate;
		DebugInfo.Score = 0.0f;
		DebugInfo.bPassedFilters = false;
		DebugInfo.bIsBest = false;
#endif

		FilterRequirement.ResetContext();
		FilterRequirement.SetContextProperty(TEXT("Instigator"), GetOwner());
		FilterRequirement.SetContextProperty(TEXT("Interactor"), this);
		FilterRequirement.SetContextProperty(TEXT("Interactable"), Candidate);
		if (!FScriptableRequirement::EvaluateRequirement(this, FilterRequirement))
		{
#if WITH_GAMEPLAY_DEBUGGER
			LastDebugCandidates.Add(DebugInfo); // Saved as Failed
#endif
			continue;
		}

		if (!Candidate->CanInteract(this))
		{
#if WITH_GAMEPLAY_DEBUGGER
			LastDebugCandidates.Add(DebugInfo); // Saved as Failed
#endif
			continue;
		}

		const FKzTransformSource CandidateTransformSource = Candidate->bRequiresInteractionSpot ? Candidate->InteractionSpot.ToTransformSource(Candidate) : FKzTransformSource(Candidate);

		float Score = UKzTargetScoringLibrary::EvaluateTarget(AsTransformSource, CandidateTransformSource, ScoringProfile);

#if WITH_GAMEPLAY_DEBUGGER
		DebugInfo.bPassedFilters = true;
		DebugInfo.Score = Score;
		LastDebugCandidates.Add(DebugInfo);
#endif

		if (Score > BestScore)
		{
			BestScore = Score;
			BestCandidate = Candidate;
		}
	}

#if WITH_GAMEPLAY_DEBUGGER
	if (BestCandidate)
	{
		for (FKzInteractionDebugCandidate& Dbg : LastDebugCandidates)
		{
			if (Dbg.Interactable == BestCandidate)
			{
				Dbg.bIsBest = true;
				break;
			}
		}
	}
#endif

	UKzInteractableComponent* OldInteractable = CurrentFocus.Get();

	if (OldInteractable != BestCandidate)
	{
		if (OldInteractable)
		{
			OldInteractable->OnEndFocus.Broadcast(this);
		}

		if (BestCandidate)
		{
			BestCandidate->OnBeginFocus.Broadcast(this);
		}

		CurrentFocus = BestCandidate;
		OnCurrentInteractableChanged.Broadcast(BestCandidate, OldInteractable);

		if (BestCandidate && BestCandidate->bIsAutomaticInteraction && !BestCandidate->bTriggerRepeatedly)
		{
			Interact();
		}
	}
	else if (BestCandidate && BestCandidate->bIsAutomaticInteraction && BestCandidate->bTriggerRepeatedly)
	{
		Interact();
	}

	// Polled every scan rather than only on focus change: what blocks an interaction can appear or clear
	// while the player stands still, and the prompt has to follow
	UpdateFocusAvailability();
}

void UKzInteractorComponent::UpdateFocusAvailability()
{
	UKzInteractableComponent* Focus = CurrentFocus.Get();

	bool bAvailable = true;
	FGameplayTag Reason;

	if (Focus)
	{
		bAvailable = Focus->GetAvailability(this, Reason);
	}

	// Only the focus is polled, so this costs one evaluation per scan no matter how crowded the room is
	if (bAvailable == bFocusAvailable && Reason == FocusUnavailableReason)
	{
		return;
	}

	bFocusAvailable = bAvailable;
	FocusUnavailableReason = Reason;

	OnFocusAvailabilityChanged.Broadcast(Focus, bAvailable, Reason);
}

EKzInteractionResult UKzInteractorComponent::Interact()
{
	return InteractWith(CurrentFocus.Get());
}

EKzInteractionResult UKzInteractorComponent::InteractWith(UKzInteractableComponent* Target)
{
	UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr;
	if (!Target || !Subsystem)
	{
		return EKzInteractionResult::Ignored;
	}

	if (!HasInteractionAuthority())
	{
		// Ask and wait: a wrong guess is worse than a late answer for something the player gets locked into
		ServerInteractWith(Target);
		return EKzInteractionResult::Pending;
	}

	FKzInteractionHandle Handle;
	const EKzInteractionResult Result = Subsystem->BeginInteraction(this, Target, Handle);

	// CurrentInteraction and scanning are already handled by HandleInteractionBegun
	if (Handle.IsValid())
	{
		UpdateReplicatedInteraction(Target, EKzInteractionEndReason::Released);
	}

	return Result;
}

void UKzInteractorComponent::ServerInteractWith_Implementation(UKzInteractableComponent* Target)
{
	// Runs the authoritative path, which validates it like any other request
	InteractWith(Target);
}

void UKzInteractorComponent::ServerEndInteraction_Implementation(EKzInteractionEndReason Reason)
{
	EndCurrentInteraction(Reason);
}

void UKzInteractorComponent::PauseScanning()
{
	// Stops the timer but leaves 'Focus' intact so the UI prompt remains on screen
	GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
}

void UKzInteractorComponent::ResumeScanning()
{
	StartScanning();
}

void UKzInteractorComponent::StopCurrentInteraction()
{
	EndCurrentInteraction(EKzInteractionEndReason::Released);
}

void UKzInteractorComponent::EndCurrentInteraction(EKzInteractionEndReason Reason)
{
	if (!HasInteractionAuthority())
	{
		ServerEndInteraction(Reason);
		return;
	}

	if (UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr)
	{
		// The rest happens in HandleInteractionEnded, which also runs when somebody else ends it
		Subsystem->EndInteraction(CurrentInteraction, Reason);
	}
}

UKzInteractableComponent* UKzInteractorComponent::GetActiveInteractable() const
{
	const UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr;
	const FKzInteraction* Interaction = Subsystem ? Subsystem->FindInteraction(CurrentInteraction) : nullptr;

	return Interaction ? Interaction->Interactable.Get() : nullptr;
}

bool UKzInteractorComponent::HasInteractionAuthority() const
{
	const UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr;
	return Subsystem ? Subsystem->HasInteractionAuthority() : true;
}

void UKzInteractorComponent::UpdateReplicatedInteraction(UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReplicatedInteraction.Interactable = Interactable;
	ReplicatedInteraction.LastEndReason = Reason;
	ReplicatedInteraction.Sequence++;
}

void UKzInteractorComponent::MirrorServerInteraction(UKzInteractableComponent* Target)
{
	UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr;
	if (!Subsystem || !Target)
	{
		return;
	}

	FKzInteractionHandle Handle;
	Subsystem->BeginInteraction(this, Target, Handle);
}

void UKzInteractorComponent::OnRep_ReplicatedInteraction(const FKzReplicatedInteraction& OldValue)
{
	UKzInteractableComponent* ServerTarget = ReplicatedInteraction.Interactable;
	UKzInteractableComponent* LocalTarget = GetActiveInteractable();

	// A new sequence on the same target means the old interaction ended and another began
	const bool bSameInteraction = LocalTarget == ServerTarget && OldValue.Sequence == ReplicatedInteraction.Sequence;
	if (bSameInteraction)
	{
		return;
	}

	if (LocalTarget)
	{
		if (UKzInteractionSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UKzInteractionSubsystem>() : nullptr)
		{
			Subsystem->EndInteraction(CurrentInteraction, ReplicatedInteraction.LastEndReason);
		}
	}

	if (ServerTarget)
	{
		MirrorServerInteraction(ServerTarget);
	}
}

void UKzInteractorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Simulated proxies need it too, to play the cosmetic side of somebody else's interaction
	DOREPLIFETIME(UKzInteractorComponent, ReplicatedInteraction);
}

void UKzInteractorComponent::HandleInteractionBegun(const FKzInteraction& Interaction)
{
	if (Interaction.Interactor.Get() != this)
	{
		return;
	}

	// Learned here rather than from BeginInteraction's return value: this runs while that call is still
	// on the stack, and whoever reacts to the event below must already see us as engaged
	CurrentInteraction = Interaction.Handle;

	// Fully stop scanning and clear UI focus since we are now locked in
	StopScanning();

	if (!InteractionBegunEventTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.Instigator = GetOwner();
	Payload.Target = Interaction.Target.Get();
	Payload.EventTag = InteractionBegunEventTag;
	Payload.OptionalObject = Interaction.Interactable.Get();

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), InteractionBegunEventTag, Payload);
}

void UKzInteractorComponent::HandleInteractionEnded(const FKzInteraction& Interaction, EKzInteractionEndReason Reason)
{
	if (Interaction.Handle != CurrentInteraction)
	{
		return;
	}

	CurrentInteraction.Invalidate();

	// Back to looking around, whatever ended it
	StartScanning();

	UpdateReplicatedInteraction(nullptr, Reason);

	OnInteractionEnded.Broadcast(Interaction.Interactable.Get(), Reason);

	if (InteractionEndedEventTag.IsValid())
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetOwner();
		Payload.Target = Interaction.Target.Get();
		Payload.EventTag = InteractionEndedEventTag;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), InteractionEndedEventTag, Payload);
	}
}
