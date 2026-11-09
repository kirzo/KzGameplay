// Copyright 2026 kirzo

#include "KzAIController.h"
#include "StateTree/KzStateTreeComponent.h"
#include "Navigation/KzPathFollowingComponent.h"
#include "Abilities/KzAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

AKzAIController::AKzAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UKzPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	StateTreeComponent = CreateDefaultSubobject<UKzStateTreeComponent>(TEXT("StateTreeComponent"));

	// Default behavior matches standard UE5 AI practices
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
}

void AKzAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PawnAbilitySystem = Cast<UKzAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InPawn));
	if (PawnAbilitySystem.IsValid())
	{
		PawnAbilitySystem->OnGameplayEvent.AddUniqueDynamic(this, &ThisClass::HandlePawnGameplayEvent);
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