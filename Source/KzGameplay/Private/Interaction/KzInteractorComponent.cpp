// Copyright 2026 kirzo

#include "Interaction/KzInteractorComponent.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Scoring/KzTargetScoringLibrary.h"

UKzInteractorComponent::UKzInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // We use timers
	ScanRate = 0.1f;

	FilterRequirement.AddContextProperty<AActor*>(TEXT("Instigator"));
	FilterRequirement.AddContextProperty<UKzInteractorComponent*>(TEXT("Interactor"));
	FilterRequirement.AddContextProperty<UKzInteractableComponent*>(TEXT("Interactable"));
}

void UKzInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

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
	GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void UKzInteractorComponent::PerformScan()
{
	UKzInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UKzInteractionSubsystem>();
	if (!Subsystem) return;

	// 1. Query the Grid (Broad + Narrow phase done internally!)
	FTransform WorldTransform = GetComponentTransform();
	TArray<UKzInteractableComponent*> Candidates = Subsystem->QueryInteractables(Shape, WorldTransform.GetLocation(), WorldTransform.GetRotation());

	UKzInteractableComponent* BestCandidate = nullptr;
	float BestScore = -1.0f;

	// 2. Evaluate Candidates
	for (UKzInteractableComponent* Candidate : Candidates)
	{
		// 2A. Filter
		FilterRequirement.ResetContext();
		FilterRequirement.SetContextProperty(TEXT("Instigator"), GetOwner());
		FilterRequirement.SetContextProperty(TEXT("Interactor"), this);
		FilterRequirement.SetContextProperty(TEXT("Interactable"), Candidate);
		if (!FScriptableRequirement::EvaluateRequirement(this, FilterRequirement))
		{
			continue;
		}

		Candidate->InteractionRequirement.ResetContext();
		Candidate->InteractionRequirement.SetContextProperty(TEXT("Instigator"), GetOwner());
		Candidate->InteractionRequirement.SetContextProperty(TEXT("Interactor"), this);
		Candidate->InteractionRequirement.SetContextProperty(TEXT("Interactable"), Candidate);
		if (!FScriptableRequirement::EvaluateRequirement(this, Candidate->InteractionRequirement))
		{
			continue;
		}

		// 2B. Soft Scoring
		float Score = UKzTargetScoringLibrary::EvaluateTarget(GetOwner(), Candidate->GetOwner(), ScoringProfile);

		// Keep the highest scorer
		if (Score > BestScore)
		{
			BestScore = Score;
			BestCandidate = Candidate;
		}
	}

	// 3. Handle State Changes
	UKzInteractableComponent* OldInteractable = CurrentFocus.Get();

	if (OldInteractable != BestCandidate)
	{
		// Unfocus the old one
		if (OldInteractable)
		{
			OldInteractable->OnEndFocus.Broadcast(this);
		}

		// CurrentFocus the new one
		if (BestCandidate)
		{
			BestCandidate->OnBeginFocus.Broadcast(this);
		}

		CurrentFocus = BestCandidate;
		OnCurrentInteractableChanged.Broadcast(BestCandidate, OldInteractable);

		// Automatic Trigger
		if (BestCandidate && BestCandidate->bIsAutomaticInteraction && !BestCandidate->bTriggerRepeatedly)
		{
			Interact();
		}
	}
	else if (BestCandidate && BestCandidate->bIsAutomaticInteraction && BestCandidate->bTriggerRepeatedly)
	{
		// Trigger Repeatedly while focused
		Interact();
	}
}

EKzInteractionResult UKzInteractorComponent::Interact()
{
	return InteractWith(CurrentFocus.Get());
}

EKzInteractionResult UKzInteractorComponent::InteractWith(UKzInteractableComponent* Target)
{
	if (!Target)
	{
		return EKzInteractionResult::Ignored;
	}

	EKzInteractionResult Result = Target->ExecuteInteraction(this);

	if (Result == EKzInteractionResult::Continuous)
	{
		ActiveInteractable = Target;

		// Fully stop scanning and clear UI focus since we are now locked in
		StopScanning();
	}

	return Result;
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
	if (ActiveInteractable)
	{
		ActiveInteractable->StopInteraction(this);
		ActiveInteractable = nullptr;

		// The interaction is over, resume scanning the environment
		StartScanning();
	}
}

void UKzInteractorComponent::Server_TryInteract_Implementation(UKzInteractableComponent* Target)
{
	InteractWith(Target);
}

void UKzInteractorComponent::Server_StopCurrentInteraction_Implementation()
{
	if (ActiveInteractable)
	{
		ActiveInteractable->StopInteraction(this);
		ActiveInteractable = nullptr;

		// Resume scanning on the server (important if it's an AI)
		StartScanning();
	}
}