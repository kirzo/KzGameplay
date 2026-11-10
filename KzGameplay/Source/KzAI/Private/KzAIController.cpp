// Copyright 2026 kirzo

#include "KzAIController.h"
#include "StateTree/KzStateTreeComponent.h"
#include "Navigation/KzPathFollowingComponent.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "Factions/KzFactionComponent.h"
#include "Sensors/KzSensorTags.h"
#include "Kismet/KzSystemLibrary.h"
#include "KzAITags.h"
#include "AbilitySystemGlobals.h"

AKzAIController::AKzAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UKzPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	StateTreeComponent = CreateDefaultSubobject<UKzStateTreeComponent>(TEXT("StateTreeComponent"));
	FactionComponent = CreateDefaultSubobject<UKzFactionComponent>(TEXT("FactionComponent"));

	// The tree usually comes from the pawn, which may have started it already: the logic starts once, in OnPossess
	bStartAILogicOnPossess = false;
	bStopAILogicOnUnposses = true;
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void AKzAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PawnAbilitySystem = Cast<UKzAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InPawn));
	if (PawnAbilitySystem.IsValid())
	{
		PawnAbilitySystem->OnGameplayEvent.AddUniqueDynamic(this, &ThisClass::HandlePawnGameplayEvent);
	}

	PerceptionSensor = UKzSystemLibrary::FindComponentByGameplayTag<UKzSensorComponent>(InPawn, KzTags::Sensor::Perception);
	if (UKzSensorComponent* Sensor = PerceptionSensor.Get())
	{
		Sensor->OnObjectBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleSenseBegin);
		Sensor->OnObjectEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleSenseEnd);
		bActivatedPerception = !Sensor->IsActive();
		Sensor->Activate();
	}

	// A tree set on the controller itself; without one this does nothing
	if (!StateTreeComponent->IsRunning())
	{
		StateTreeComponent->StartLogic();
	}
}

void AKzAIController::OnUnPossess()
{
	if (PawnAbilitySystem.IsValid())
	{
		PawnAbilitySystem->OnGameplayEvent.RemoveDynamic(this, &ThisClass::HandlePawnGameplayEvent);
	}
	PawnAbilitySystem.Reset();

	if (UKzSensorComponent* Sensor = PerceptionSensor.Get())
	{
		Sensor->OnObjectBeginOverlap.RemoveDynamic(this, &ThisClass::HandleSenseBegin);
		Sensor->OnObjectEndOverlap.RemoveDynamic(this, &ThisClass::HandleSenseEnd);
		if (bActivatedPerception)
		{
			Sensor->Deactivate();
		}
	}
	PerceptionSensor.Reset();
	bActivatedPerception = false;

	Super::OnUnPossess();
}

void AKzAIController::HandlePawnGameplayEvent(const FGameplayTag& Tag, FGameplayEventData EventData)
{
	StateTreeComponent->SendStateTreeEvent(Tag, FConstStructView::Make(EventData));
}

void AKzAIController::HandleSenseBegin(const FKzOverlapResult& OverlapInfo)
{
	StateTreeComponent->SendStateTreeEvent(KzTags::AI::Sense_Begin, FConstStructView::Make(OverlapInfo));
}

void AKzAIController::HandleSenseEnd(const FKzOverlapResult& OverlapInfo)
{
	StateTreeComponent->SendStateTreeEvent(KzTags::AI::Sense_End, FConstStructView::Make(OverlapInfo));
}