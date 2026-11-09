// Copyright 2026 kirzo

#include "KzAIController.h"
#include "StateTree/KzStateTreeComponent.h"
#include "Navigation/KzPathFollowingComponent.h"

AKzAIController::AKzAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UKzPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	StateTreeComponent = CreateDefaultSubobject<UKzStateTreeComponent>(TEXT("StateTreeComponent"));

	// Default behavior matches standard UE5 AI practices
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;
}
