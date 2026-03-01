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

	// Only scan on the local client (or the server if this is an AI).
	// We don't want simulated proxies (other players on your screen) running scans.
	if (GetOwner()->GetLocalRole() != ROLE_SimulatedProxy)
	{
		GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UKzInteractorComponent::PerformScan, ScanRate, true);
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
	UKzInteractableComponent* OldInteractable = CurrentInteractable.Get();

	if (OldInteractable != BestCandidate)
	{
		// Unfocus the old one
		if (OldInteractable)
		{
			OldInteractable->OnEndFocus.Broadcast(this);
		}

		// Focus the new one
		if (BestCandidate)
		{
			BestCandidate->OnBeginFocus.Broadcast(this);
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

bool UKzInteractorComponent::Interact()
{
	UKzInteractableComponent* Target = CurrentInteractable.Get();

	if (Target)
	{
		// If we are the server, execute immediately. If client, ask the server.
		if (GetOwner()->HasAuthority())
		{
			Target->ExecuteInteraction(this);
		}
		else
		{
			Server_TryInteract(Target);
		}

		return true;
	}

	return false;
}

bool UKzInteractorComponent::Server_TryInteract_Validate(UKzInteractableComponent* Target)
{
	return IsValid(Target);
}

void UKzInteractorComponent::Server_TryInteract_Implementation(UKzInteractableComponent* Target)
{
	if (Target)
	{
		// In a highly secure game, you would re-run a distance/LoS check here to ensure 
		// the client didn't hack the RPC to interact with a chest 5 miles away.
		// For now, we trust the client's target and execute:
		Target->ExecuteInteraction(this);
	}
}