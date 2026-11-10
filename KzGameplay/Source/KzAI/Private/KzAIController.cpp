// Copyright 2026 kirzo

#include "KzAIController.h"
#include "StateTree/KzStateTreeComponent.h"
#include "Navigation/KzPathFollowingComponent.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "Factions/KzFactionComponent.h"
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

	Super::OnUnPossess();
}

void AKzAIController::HandlePawnGameplayEvent(const FGameplayTag& Tag, FGameplayEventData EventData)
{
	StateTreeComponent->SendStateTreeEvent(Tag, FConstStructView::Make(EventData));
}