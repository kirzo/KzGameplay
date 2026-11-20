// Copyright 2026 kirzo

#include "Cinematics/ScriptableTask_CinematicMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UScriptableTask_CinematicMode::BeginTask()
{
	APlayerController* Controller = Cast<APlayerController>(Player);
	if (const APawn* Pawn = Cast<APawn>(Player))
	{
		Controller = Cast<APlayerController>(Pawn->GetController());
	}

	if (!Controller)
	{
		Finish();
		return;
	}

	AppliedTo = Controller;
	UKzCinematicSubsystem::PushCinematicMode(Controller, this, Settings);
}

void UScriptableTask_CinematicMode::StopTask()
{
	UKzCinematicSubsystem::RemoveCinematicMode(AppliedTo.Get(), this);
	AppliedTo.Reset();
}