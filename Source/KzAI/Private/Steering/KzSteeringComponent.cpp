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

void UKzSteeringComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 1. Check Owner
	AgentInterface = Cast<IKzSteeringAgent>(Owner);

	// 2. Check Components (useful if a NavMovementComponent implements the interface)
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

	if (!AgentInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("UKzSteeringComponent requires the owner or a component to implement IKzSteeringAgent."));
		SetComponentTickEnabled(false);
	}
}

void UKzSteeringComponent::PushProfile(UKzSteeringProfile* Profile, FGameplayTag LayerTag, int32 Priority)
{
	if (!Profile || !AgentInterface) return;

	FKzSteeringLayer NewLayer;
	NewLayer.LayerTag = LayerTag;
	NewLayer.Priority = Priority;

	for (const UKzSteeringBehavior* TemplateBehavior : Profile->Behaviors)
	{
		if (TemplateBehavior)
		{
			UKzSteeringBehavior* NewBehavior = DuplicateObject<UKzSteeringBehavior>(TemplateBehavior, this);
			NewBehavior->InitBehavior(this, AgentInterface);
			NewLayer.Behaviors.Add(NewBehavior);
		}
	}

	LayerStack.Push(NewLayer, LayerTag, Priority);
}

void UKzSteeringComponent::PushBehavior(UKzSteeringBehavior* PreconfiguredBehavior, FGameplayTag LayerTag, int32 Priority)
{
	if (!PreconfiguredBehavior || !AgentInterface) return;

	FKzSteeringLayer NewLayer;
	NewLayer.LayerTag = LayerTag;
	NewLayer.Priority = Priority;

	// Duplicate the passed object so the component owns its own unique, safe instance
	UKzSteeringBehavior* NewBehavior = DuplicateObject<UKzSteeringBehavior>(PreconfiguredBehavior, this);
	NewBehavior->InitBehavior(this, AgentInterface);
	NewLayer.Behaviors.Add(NewBehavior);

	LayerStack.Push(NewLayer, LayerTag, Priority);
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

	FVector TotalForce = FVector::ZeroVector;
	float ForceMagnitudeLeft = MaxSteeringForce;

	for (const FKzSteeringLayer* Layer : SortedLayers)
	{
		FVector LayerForce = FVector::ZeroVector;

		for (UKzSteeringBehavior* Behavior : Layer->Behaviors)
		{
			if (Behavior && Behavior->Weight > 0.0f)
			{
				LayerForce += Behavior->ComputeForce(this, AgentInterface, DeltaTime) * Behavior->Weight;
			}
		}

		float LayerForceMag = LayerForce.Size();
		if (LayerForceMag > UE_KINDA_SMALL_NUMBER)
		{
			// Prioritized Truncated Sum Math
			if (LayerForceMag > ForceMagnitudeLeft)
			{
				TotalForce += (LayerForce / LayerForceMag) * ForceMagnitudeLeft;
				ForceMagnitudeLeft = 0.0f;
				break; // The highest priority layer ate all the budget. Skip lower layers.
			}
			else
			{
				TotalForce += LayerForce;
				ForceMagnitudeLeft -= LayerForceMag;
			}
		}
	}

	if (!TotalForce.IsNearlyZero())
	{
		const float MaxAccel = AgentInterface->GetAgentMaxAcceleration();
		const FVector MovementInput = TotalForce / FMath::Max(MaxAccel, 1.0f);

		AgentInterface->ApplySteeringInput(MovementInput);
	}
}