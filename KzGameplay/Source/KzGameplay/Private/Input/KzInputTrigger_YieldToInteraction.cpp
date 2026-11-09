// Copyright 2026 kirzo

#include "Input/KzInputTrigger_YieldToInteraction.h"
#include "Interaction/KzInteractorComponent.h"
#include "Kismet/KzSystemLibrary.h"

#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ETriggerState UKzInputTrigger_YieldToInteraction::UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime)
{
	const bool bActuated = IsActuated(ModifiedValue);
	if (bActuated && !IsActuated(LastValue))
	{
		const APlayerController* Controller = PlayerInput ? Cast<APlayerController>(PlayerInput->GetOuter()) : nullptr;
		const UKzInteractorComponent* Interactor = Controller ? UKzSystemLibrary::FindComponentInActorOrController<UKzInteractorComponent>(Controller->GetPawn()) : nullptr;
		bPressYielded = Interactor && (Interactor->GetCurrentFocus() || Interactor->IsInteractingContinuously());
	}
	return bActuated && !bPressYielded ? ETriggerState::Triggered : ETriggerState::None;
}
