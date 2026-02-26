// Copyright 2026 kirzo

#include "Interaction/InteractorComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Scoring/TargetScoringLibrary.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // We use timers
	ScanRate = 0.1f;
}

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only scan on the local client (or the server if this is an AI).
	// We don't want simulated proxies (other players on your screen) running scans.
	if (GetOwner()->GetLocalRole() != ROLE_SimulatedProxy)
	{
		GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UInteractorComponent::PerformScan, ScanRate, true);
	}
}

void UInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void UInteractorComponent::PerformScan()
{
	UInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UInteractionSubsystem>();
	if (!Subsystem) return;

	// 1. Query the Grid (Broad + Narrow phase done internally!)
	FTransform WorldTransform = GetComponentTransform();
	TArray<UInteractableComponent*> Candidates = Subsystem->QueryInteractables(Shape, WorldTransform.GetLocation(), WorldTransform.GetRotation());

	UInteractableComponent* BestCandidate = nullptr;
	float BestScore = -1.0f;

	// 2. Evaluate Candidates
	for (UInteractableComponent* Candidate : Candidates)
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

		// 2B. Soft Scoring
		float Score = UTargetScoringLibrary::EvaluateTarget(GetOwner(), Candidate->GetOwner(), ScoringProfile);

		// Keep the highest scorer
		if (Score > BestScore)
		{
			BestScore = Score;
			BestCandidate = Candidate;
		}
	}

	// 3. Handle State Changes
	UInteractableComponent* OldInteractable = CurrentInteractable.Get();

	if (OldInteractable != BestCandidate)
	{
		// Unfocus the old one
		if (OldInteractable)
		{
			OldInteractable->OnEndFocus.Broadcast(GetOwner());
		}

		// Focus the new one
		if (BestCandidate)
		{
			BestCandidate->OnBeginFocus.Broadcast(GetOwner());
		}

		CurrentInteractable = BestCandidate;
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

void UInteractorComponent::Interact()
{
	UInteractableComponent* Target = CurrentInteractable.Get();

	if (Target)
	{
		// If we are the server, execute immediately. If client, ask the server.
		if (GetOwner()->HasAuthority())
		{
			Target->ExecuteInteraction(GetOwner());
		}
		else
		{
			Server_TryInteract(Target);
		}
	}
}

bool UInteractorComponent::Server_TryInteract_Validate(UInteractableComponent* Target)
{
	return IsValid(Target);
}

void UInteractorComponent::Server_TryInteract_Implementation(UInteractableComponent* Target)
{
	if (Target)
	{
		// In a highly secure game, you would re-run a distance/LoS check here to ensure 
		// the client didn't hack the RPC to interact with a chest 5 miles away.
		// For now, we trust the client's target and execute:
		Target->ExecuteInteraction(GetOwner());
	}
}