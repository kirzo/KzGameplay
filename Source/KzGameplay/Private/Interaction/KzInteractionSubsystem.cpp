// Copyright 2026 kirzo

#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractorComponent.h"
#include "Components/KzShapeComponent.h"
#include "Engine/World.h"

// =================================================================
// SEMANTICS IMPLEMENTATION
// =================================================================

FBox FInteractionGridSemantics::GetBoundingBox(const UKzInteractableComponent* E)
{
	return E ? E->Shape.GetBoundingBox(E->GetComponentTransform()) : FBox(EForceInit::ForceInit);
}

UKzInteractableComponent* FInteractionGridSemantics::GetElementId(const UKzInteractableComponent* E)
{
	return const_cast<UKzInteractableComponent*>(E);
}

bool FInteractionGridSemantics::IsValid(const UKzInteractableComponent* E)
{
	return ::IsValid(E);
}

FVector FInteractionGridSemantics::GetElementPosition(const UKzInteractableComponent* E)
{
	return E ? E->GetComponentLocation() : FVector::ZeroVector;
}

FKzShapeInstance FInteractionGridSemantics::GetShape(const UKzInteractableComponent* E)
{
	return E ? E->Shape : FKzShapeInstance();
}

FQuat FInteractionGridSemantics::GetElementRotation(const UKzInteractableComponent* E)
{
	return E ? E->GetComponentQuat() : FQuat::Identity;
}

bool FInteractionGridSemantics::IsDynamic(const UKzInteractableComponent* E)
{
	return E && E->bIsDynamicInteraction;
}

// =================================================================
// SUBSYSTEM IMPLEMENTATION
// =================================================================

void UKzInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Registry.SetCellSize(GridCellSize);
}

void UKzInteractionSubsystem::Deinitialize()
{
	// The world is going away with everyone in it
	Interactions.Empty();

	Registry.Reset();
	Super::Deinitialize();
}

TStatId UKzInteractionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzInteractionSubsystem, STATGROUP_Tickables);
}

EKzInteractionResult UKzInteractionSubsystem::BeginInteraction(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, FKzInteractionHandle& OutHandle)
{
	OutHandle.Invalidate();

	if (!IsValid(Interactor) || !IsValid(Interactable))
	{
		return EKzInteractionResult::Ignored;
	}

	// Checked here rather than inside the target, so no path can skip it. A client mirroring what the
	// server already decided does not get to second-guess it.
	if (HasInteractionAuthority())
	{
		FGameplayTag UnavailableReason;
		if (!Interactable->CanInteract(Interactor) || !Interactable->GetAvailability(Interactor, UnavailableReason))
		{
			return EKzInteractionResult::Ignored;
		}
	}

	// Decided before anything exists, so a refusal leaves no trace and no handler has acted yet
	const EKzInteractionResult Result = Interactable->EvaluateInteractionResult(Interactor);
	if (Result == EKzInteractionResult::Ignored)
	{
		return EKzInteractionResult::Ignored;
	}

	FKzInteraction Interaction;
	Interaction.Handle = FKzInteractionHandle(NextHandleId++);
	Interaction.Interactor = Interactor;
	Interaction.Interactable = Interactable;
	Interaction.Instigator = Interactor->GetOwner();
	Interaction.Target = Interactable->GetOwner();
	Interaction.StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	Interaction.Scope = MakeShared<FKzInteractionScope>();

	// Registered first, so anything the handlers do already sees the interaction
	const FKzInteractionHandle Handle = Interaction.Handle;
	Interactions.Add(Handle, Interaction);

	// The interactor first, and directly: handlers below can activate abilities that ask it what it is
	// interacting with, and it has to have an answer by then
	Interactor->HandleInteractionBegun(Interactions[Handle]);

	Interactable->NotifyInteractionBegun(Interactor, Interactions[Handle]);
	OnInteractionBegun.Broadcast(Interactions[Handle]);

	if (Result == EKzInteractionResult::Completed)
	{
		EndInteraction(Handle, EKzInteractionEndReason::Completed);
		return Result;
	}

	OutHandle = Handle;
	return Result;
}

void UKzInteractionSubsystem::EndInteraction(FKzInteractionHandle Handle, EKzInteractionEndReason Reason)
{
	FKzInteraction Interaction;
	if (!Interactions.RemoveAndCopyValue(Handle, Interaction))
	{
		// Ending twice is normal: whoever ended it and whoever reacted both try
		return;
	}

	// Undo before anyone reacts, so listeners see a settled world
	if (Interaction.Scope.IsValid())
	{
		Interaction.Scope->Run();
	}

	// Already removed, so a listener calling back into us finds it gone instead of recursing
	if (UKzInteractableComponent* Interactable = Interaction.Interactable.Get())
	{
		Interactable->NotifyInteractionEnded(Interaction, Reason);
	}

	OnInteractionEnded.Broadcast(Interaction, Reason);
}

void UKzInteractionSubsystem::EndInteractionsFor(const UKzInteractorComponent* Interactor, EKzInteractionEndReason Reason)
{
	TArray<FKzInteractionHandle> Handles;
	for (const TPair<FKzInteractionHandle, FKzInteraction>& Pair : Interactions)
	{
		if (Pair.Value.Interactor.Get() == Interactor)
		{
			Handles.Add(Pair.Key);
		}
	}

	for (const FKzInteractionHandle& Handle : Handles)
	{
		EndInteraction(Handle, Reason);
	}
}

void UKzInteractionSubsystem::EndInteractionsOn(const UKzInteractableComponent* Interactable, EKzInteractionEndReason Reason)
{
	TArray<FKzInteractionHandle> Handles;
	for (const TPair<FKzInteractionHandle, FKzInteraction>& Pair : Interactions)
	{
		if (Pair.Value.Interactable.Get() == Interactable)
		{
			Handles.Add(Pair.Key);
		}
	}

	for (const FKzInteractionHandle& Handle : Handles)
	{
		EndInteraction(Handle, Reason);
	}
}

FKzInteractionHandle UKzInteractionSubsystem::FindInteractionFor(const UKzInteractorComponent* Interactor) const
{
	for (const TPair<FKzInteractionHandle, FKzInteraction>& Pair : Interactions)
	{
		if (Pair.Value.Interactor.Get() == Interactor)
		{
			return Pair.Key;
		}
	}

	return FKzInteractionHandle();
}

TArray<FKzInteractionHandle> UKzInteractionSubsystem::FindInteractionsOn(const UKzInteractableComponent* Interactable) const
{
	TArray<FKzInteractionHandle> Handles;
	for (const TPair<FKzInteractionHandle, FKzInteraction>& Pair : Interactions)
	{
		if (Pair.Value.Interactable.Get() == Interactable)
		{
			Handles.Add(Pair.Key);
		}
	}

	return Handles;
}

int32 UKzInteractionSubsystem::CountInteractionsOn(const UKzInteractableComponent* Interactable) const
{
	int32 Count = 0;
	for (const TPair<FKzInteractionHandle, FKzInteraction>& Pair : Interactions)
	{
		if (Pair.Value.Interactable.Get() == Interactable)
		{
			Count++;
		}
	}

	return Count;
}

bool UKzInteractionSubsystem::IsActorInteractingWith(const AActor* Actor, const UKzInteractableComponent* Interactable) const
{
	if (!Actor)
	{
		return false;
	}

	for (const TPair<FKzInteractionHandle, FKzInteraction>& Pair : Interactions)
	{
		if (Pair.Value.Instigator.Get() == Actor && Pair.Value.Interactable.Get() == Interactable)
		{
			return true;
		}
	}

	return false;
}

void UKzInteractionSubsystem::RegisterInteractable(UKzInteractableComponent* Component)
{
	Registry.Register(Component);
}

void UKzInteractionSubsystem::UnregisterInteractable(UKzInteractableComponent* Component)
{
	// Nothing else in the engine tells the interactors that their target is going away
	EndInteractionsOn(Component, EKzInteractionEndReason::TargetLost);

	Registry.Unregister(Component);
}

TArray<UKzInteractableComponent*> UKzInteractionSubsystem::QueryInteractables(const FKzShapeInstance& QueryShape, const FVector& ShapePosition, const FQuat& ShapeRotation) const
{
	TArray<UKzInteractableComponent*> Results;
	Registry.Query(Results, QueryShape, ShapePosition, ShapeRotation);
	return Results;
}

bool UKzInteractionSubsystem::HasInteractionAuthority() const
{
	const UWorld* World = GetWorld();
	return !World || World->GetNetMode() != NM_Client;
}

void UKzInteractionSubsystem::ValidateInteractions()
{
	// Only the authority decides when an interaction stops holding; clients are told
	if (!HasInteractionAuthority() || bValidating)
	{
		return;
	}

	TGuardValue<bool> Guard(bValidating, true);

	TArray<TPair<FKzInteractionHandle, EKzInteractionEndReason>> Expired;

	for (const TPair<FKzInteractionHandle, FKzInteraction>& Pair : Interactions)
	{
		const FKzInteraction& Interaction = Pair.Value;

		if (!Interaction.Interactable.IsValid() || !Interaction.Target.IsValid())
		{
			Expired.Emplace(Pair.Key, EKzInteractionEndReason::TargetLost);
			continue;
		}

		if (!Interaction.Interactor.IsValid() || !Interaction.Instigator.IsValid())
		{
			Expired.Emplace(Pair.Key, EKzInteractionEndReason::InstigatorLost);
			continue;
		}

		EKzInteractionEndReason Reason = EKzInteractionEndReason::ConditionFailed;
		if (!Interaction.Interactable->ShouldKeepInteractionAlive(Interaction, Reason))
		{
			Expired.Emplace(Pair.Key, Reason);
		}
	}

	for (const TPair<FKzInteractionHandle, EKzInteractionEndReason>& Pair : Expired)
	{
		EndInteraction(Pair.Key, Pair.Value);
	}
}

void UKzInteractionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Registry.TickDynamics();

	TimeUntilValidation -= DeltaTime;
	if (TimeUntilValidation <= 0.0f)
	{
		TimeUntilValidation = ValidationInterval;
		ValidateInteractions();
	}
}
