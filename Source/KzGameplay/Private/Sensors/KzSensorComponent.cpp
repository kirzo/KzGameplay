// Copyright 2026 kirzo

#include "Sensors/KzSensorComponent.h"
#include "Sensors/KzSensableComponent.h"
#include "Sensors/KzSpatialSenseSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

UKzSensorComponent::UKzSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // We use timers instead for performance
	bDrawSolid = false;

	// Initialize requirement context properties
	DetectionRequirement.AddContextProperty<AActor*>(TEXT("Instigator"));
	DetectionRequirement.AddContextProperty<AActor*>(TEXT("Target"));
}

void UKzSensorComponent::GetSensedActors(TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors) const
{
	OutActors.Empty();
	if (!ActorClass) return;

	for (const FKzOverlapResult& Overlap : CachedOverlaps)
	{
		if (AActor* SensedActor = Overlap.Actor.Get())
		{
			if (SensedActor->IsA(ActorClass))
			{
				OutActors.Add(SensedActor);
			}
		}
	}
}

void UKzSensorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only run scanning on the server or standalone to avoid state desyncs in MP
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UKzSensorComponent::PerformScan, ScanInterval, true);
		}
	}
}

void UKzSensorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UKzSensorComponent::PerformScan()
{
	UKzSpatialSenseSubsystem* Subsystem = GetWorld()->GetSubsystem<UKzSpatialSenseSubsystem>();
	if (!Subsystem) return;

	// 1. Broadphase: O(1) Spatial Query + O(N) Tag Filter natively
	TArray<UKzSensableComponent*> DetectedSensables = Subsystem->QuerySensables(
		Shape * GetComponentScale(),
		GetComponentLocation(),
		GetComponentQuat(),
		SearchTagQuery
	);

	TArray<FKzOverlapResult> NewOverlaps;
	AActor* InstigatorActor = GetOwner();

	// 2. Narrowphase: Evaluate Scriptable Requirements (The logical filter)
	for (UKzSensableComponent* Candidate : DetectedSensables)
	{
		if (!Candidate) continue;

		AActor* TargetActor = Candidate->GetOwner();

		// Prevent the sensor from detecting itself, its own owner, or null actors
		if (!TargetActor || TargetActor == InstigatorActor) continue;

		// Set context for the requirement evaluator
		DetectionRequirement.ResetContext();
		DetectionRequirement.SetContextProperty(TEXT("Instigator"), InstigatorActor);
		DetectionRequirement.SetContextProperty(TEXT("Target"), TargetActor);

		if (FScriptableRequirement::EvaluateRequirement(this, DetectionRequirement))
		{
			// Ensure we don't add duplicates if multiple Sensables exist on the same Actor
			bool bAlreadyAdded = NewOverlaps.ContainsByPredicate([TargetActor](const FKzOverlapResult& Item)
				{
					return Item.Actor.Get() == TargetActor;
				});

			if (!bAlreadyAdded)
			{
				FKzOverlapResult NewResult;
				NewResult.Actor = TargetActor;
				NewResult.SensableComponent = Candidate;
				NewOverlaps.Add(NewResult);
			}
		}
	}

	// 3. Process Begin Overlaps (Entries)
	for (const FKzOverlapResult& NewResult : NewOverlaps)
	{
		AActor* NewActor = NewResult.Actor.Get();
		if (!NewActor) continue;

		bool bIsNew = true;
		for (const FKzOverlapResult& Existing : CachedOverlaps)
		{
			if (Existing.Actor.Get() == NewActor)
			{
				bIsNew = false;
				break;
			}
		}

		if (bIsNew)
		{
			LastDetectedActor = NewActor;
			OnObjectBeginOverlap.Broadcast(NewResult);
		}
	}

	// 4. Process End Overlaps (Exits)
	TArray<FKzOverlapResult> OldCache = CachedOverlaps;
	CachedOverlaps = NewOverlaps; // Swap state with the new valid frame data

	for (const FKzOverlapResult& OldResult : OldCache)
	{
		AActor* OldActor = OldResult.Actor.Get();

		bool bStillOverlapping = CachedOverlaps.ContainsByPredicate([OldActor](const FKzOverlapResult& Item)
			{
				return Item.Actor.Get() == OldActor;
			});

		// If it's no longer overlapping but the actor still exists, broadcast end overlap
		if (!bStillOverlapping && OldActor)
		{
			// Clean up the cached reference so we don't hold stale data
			if (LastDetectedActor == OldActor)
			{
				LastDetectedActor = nullptr;
			}

			OnObjectEndOverlap.Broadcast(OldResult);
		}
	}
}