// Copyright 2026 kirzo

#include "Input/KzInputTrigger_YieldToInteraction.h"
#include "Interaction/KzInteractorComponent.h"
#include "Kismet/KzSystemLibrary.h"

#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#if WITH_EDITOR
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/DataValidation.h"

EDataValidationResult UKzInputTrigger_YieldToInteraction::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);

	// The action whose key this trigger hands over: its mapping's in a mapping context, or the Input Action it sits on
	const UInputAction* Action = Cast<UInputAction>(GetOuter());
	if (const UInputMappingContext* MappingContext = Cast<UInputMappingContext>(GetOuter()))
	{
		const FEnhancedActionKeyMapping* Mapping = MappingContext->GetMappings().FindByPredicate([this](const FEnhancedActionKeyMapping& Each)
		{
			return Each.Triggers.ContainsByPredicate([this](const UInputTrigger* Trigger) { return Trigger == this; });
		});
		Action = Mapping ? Mapping->Action.Get() : nullptr;
	}

	if (Action && Action->bConsumeInput)
	{
		// A warning, not an error: it works while the interaction's context is the higher one, which nothing guarantees
		Context.AddWarning(FText::Format(INVTEXT("Yield To Interaction is on {0}, which consumes input: whenever its mapping context is above the interaction's, the interaction never gets the key. Uncheck Consume Lower Priority Enhanced Input Mappings on {0}."), FText::FromString(Action->GetName())));
	}
	return Result;
}
#endif

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