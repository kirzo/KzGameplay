// Copyright 2026 kirzo

#include "Steering/KzSteeringComponent.h"
#include "Steering/KzSteeringProfile.h"
#include "Steering/KzSteeringBehavior.h"
#include "GameFramework/Actor.h"

UKzSteeringComponent::UKzSteeringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	AgentInterface = nullptr;
}

void UKzSteeringComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	if (UKzSteeringComponent* ThisComp = Cast<UKzSteeringComponent>(InThis))
	{
		ThisComp->LayerStack.ForEach([&Collector](FKzSteeringLayer& Layer)
			{
				for (TObjectPtr<UKzSteeringBehavior>& Behavior : Layer.Behaviors)
				{
					Collector.AddReferencedObject(Behavior);
				}
			});
	}

	Super::AddReferencedObjects(InThis, Collector);
}

void UKzSteeringComponent::EnsureAgentInterface()
{
	if (AgentInterface) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 1. Check Owner
	AgentInterface = Cast<IKzSteeringAgent>(Owner);

	// 2. Check Components
	if (!AgentInterface)
	{
		for (UActorComponent* Comp : Owner->GetComponents())
		{
			if (IKzSteeringAgent* AgentComp = Cast<IKzSteeringAgent>(Comp))
			{
				AgentInterface = AgentComp;
				break;
			}
		}
	}
}

void UKzSteeringComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureAgentInterface();

	if (!AgentInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("UKzSteeringComponent requires the owner or a component to implement IKzSteeringAgent."));
	}
}

void UKzSteeringComponent::PushProfile(UKzSteeringProfile* Profile, FGameplayTag LayerTag, int32 Priority)
{
	EnsureAgentInterface();

	if (!Profile || !AgentInterface) return;

	FKzSteeringLayer* ExistingLayer = LayerStack.Find(LayerTag);
	FKzSteeringLayer NewLayer; // Used only if it doesn't exist

	// Point to the correct array of behaviors
	TArray<TObjectPtr<UKzSteeringBehavior>>* TargetBehaviors = nullptr;

	if (ExistingLayer)
	{
		TargetBehaviors = &ExistingLayer->Behaviors;
	}
	else
	{
		NewLayer.LayerTag = LayerTag;
		NewLayer.Priority = Priority;
		TargetBehaviors = &NewLayer.Behaviors;
	}

	// Instantiate all behaviors in the profile
	for (const UKzSteeringBehavior* TemplateBehavior : Profile->Behaviors)
	{
		if (TemplateBehavior)
		{
			UKzSteeringBehavior* NewBehavior = DuplicateObject<UKzSteeringBehavior>(TemplateBehavior, this);
			NewBehavior->InitBehavior(this, AgentInterface);
			TargetBehaviors->Add(NewBehavior);
		}
	}

	// Push only if it was a completely new layer
	if (!ExistingLayer)
	{
		LayerStack.Push(NewLayer, LayerTag, Priority);
	}
}

void UKzSteeringComponent::PushBehavior(UKzSteeringBehavior* PreconfiguredBehavior, FGameplayTag LayerTag, int32 Priority)
{
	EnsureAgentInterface();

	if (!PreconfiguredBehavior || !AgentInterface) return;

	// Duplicate the passed object so the component owns its own unique, safe instance
	UKzSteeringBehavior* NewBehavior = DuplicateObject<UKzSteeringBehavior>(PreconfiguredBehavior, this);
	NewBehavior->InitBehavior(this, AgentInterface);

	// Check if the layer already exists in the stack
	if (FKzSteeringLayer* ExistingLayer = LayerStack.Find(LayerTag))
	{
		ExistingLayer->Behaviors.Add(NewBehavior);

		// Note: We ignore the Priority argument here because the layer is already sorted in the Heap.
		// The layer's priority was defined by the first behavior that created it.
	}
	else
	{
		// Create a new layer from scratch
		FKzSteeringLayer NewLayer;
		NewLayer.LayerTag = LayerTag;
		NewLayer.Priority = Priority;
		NewLayer.Behaviors.Add(NewBehavior);

		LayerStack.Push(NewLayer, LayerTag, Priority);
	}
}

void UKzSteeringComponent::RemoveLayer(FGameplayTag LayerTag)
{
	LayerStack.Pop(LayerTag);
}

void UKzSteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!AgentInterface || LayerStack.IsEmpty()) return;

	TArray<FKzSteeringLayer*> SortedLayers = LayerStack.GetSortedElements();

	FVector TotalDesiredVelocity = FVector::ZeroVector;
	const float MaxSpeed = AgentInterface->GetAgentMaxSpeed();

	// We use MaxSpeed as our budget for the prioritized truncated sum
	float SpeedBudgetLeft = MaxSpeed;

	for (const FKzSteeringLayer* Layer : SortedLayers)
	{
		FVector LayerVelocity = FVector::ZeroVector;

		for (UKzSteeringBehavior* Behavior : Layer->Behaviors)
		{
			if (Behavior && Behavior->Weight > 0.0f)
			{
				// ComputeForce now acts as ComputeDesiredVelocity
				LayerVelocity += Behavior->ComputeForce(this, AgentInterface, DeltaTime) * Behavior->Weight;
			}
		}

		float LayerVelocityMag = LayerVelocity.Size();
		if (LayerVelocityMag > UE_KINDA_SMALL_NUMBER)
		{
			// Prioritized Truncated Sum Math
			if (LayerVelocityMag > SpeedBudgetLeft)
			{
				TotalDesiredVelocity += (LayerVelocity / LayerVelocityMag) * SpeedBudgetLeft;
				SpeedBudgetLeft = 0.0f;
				break; // The highest priority layer ate all the budget. Skip lower layers.
			}
			else
			{
				TotalDesiredVelocity += LayerVelocity;
				SpeedBudgetLeft -= LayerVelocityMag;
			}
		}
	}

	FVector CurrentVelocity = AgentInterface->GetAgentVelocity();

	// We only skip if the agent is completely at rest AND has no desired velocity (no orders)
	if (!TotalDesiredVelocity.IsNearlyZero() || !CurrentVelocity.IsNearlyZero())
	{
		// Calculate the ideal change in velocity needed to match the desired state
		FVector DeltaVelocity = TotalDesiredVelocity - CurrentVelocity;

		// Limit the change by our physical maximum acceleration (Physics: DeltaV = Acceleration * DeltaTime)
		const float MaxAcceleration = AgentInterface->GetAgentMaxAcceleration();
		const float MaxVelocityChangeThisFrame = MaxAcceleration * DeltaTime;

		// Clamp the DeltaVelocity so the agent doesn't stop or turn instantly, maintaining inertia
		DeltaVelocity = DeltaVelocity.GetClampedToMaxSize(MaxVelocityChangeThisFrame);

		// Calculate the actual physical velocity for this frame
		FVector NewVelocity = CurrentVelocity + DeltaVelocity;
		AgentInterface->ApplySteeringForce(NewVelocity);
	}
}