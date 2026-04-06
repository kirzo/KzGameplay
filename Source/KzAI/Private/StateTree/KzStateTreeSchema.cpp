// Copyright 2026 kirzo

#include "StateTree/KzStateTreeSchema.h"
#include "Steering/KzSteeringComponent.h"
#include "Sensors/KzSensorComponent.h"
#include "BrainComponent.h"
#include "KzAIController.h"

UKzStateTreeSchema::UKzStateTreeSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = AKzAIController::StaticClass();

	// The base UStateTreeAIComponentSchema already adds:
	// Index 0: Actor (APawn)
	// Index 1: AIController
	// We just append our custom KzAI components to the array.
	ContextDataDescs.Append({
			{ Kz::StateTree::Context::SteeringComponent, UKzSteeringComponent::StaticClass(), FGuid(0x4A5B6C7D, 0x8E9F0A1B, 0x2C3D4E5F, 0x6A7B8C9D) },
			{ Kz::StateTree::Context::SensorComponent, UKzSensorComponent::StaticClass(), FGuid(0x9D8E7F6A, 0x5B4C3D2E, 0x1F0A9B8C, 0x7D6E5F4A) }
		});
}

void UKzStateTreeSchema::SetContextData(FContextDataSetter& ContextDataSetter, bool bLogErrors) const
{
	// 1. Let the Epic base classes inject the ContextActor (Pawn) and the AIController
	Super::SetContextData(ContextDataSetter, bLogErrors);

	// 2. Extract the Pawn to find our custom components
	AActor* Pawn = nullptr;
	if (AAIController* AIOwner = ContextDataSetter.GetComponent()->GetAIOwner())
	{
		Pawn = AIOwner->GetPawn();
	}

	// 3. Inject KzAI specific components into the context
	if (Pawn)
	{
		ContextDataSetter.SetContextDataByName(
			Kz::StateTree::Context::SteeringComponent,
			FStateTreeDataView(Pawn->FindComponentByClass<UKzSteeringComponent>())
		);

		ContextDataSetter.SetContextDataByName(
			Kz::StateTree::Context::SensorComponent,
			FStateTreeDataView(Pawn->FindComponentByClass<UKzSensorComponent>())
		);
	}
}