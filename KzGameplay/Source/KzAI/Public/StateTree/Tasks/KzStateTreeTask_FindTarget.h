// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Factions/KzFactionSettings.h"
#include "KzStateTreeTask_FindTarget.generated.h"

class UKzSensorComponent;

USTRUCT()
struct FKzStateTreeTask_FindTargetInstanceData
{
	GENERATED_BODY()

	/** The pawn whose attitude counts. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** Where the candidates come from: the sensor the schema picks. Its range and requirements are what the AI perceives. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<UKzSensorComponent> Sensor = nullptr;

	/** How the pawn must see a candidate to pick it. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	EKzAttitude Attitude = EKzAttitude::Hostile;

	/** The nearest candidate, kept while the sensor still has it so the AI doesn't switch back and forth. None without one. */
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> Target = nullptr;
};

/**
 * Picks a target among what a sensor perceives, by the pawn's attitude towards it, so a tree speaks of hostiles or allies
 * instead of characters or factions. Meant as a global task: it keeps Target up to date while the tree runs. Promoted to a
 * parameter, the other nodes and linked subtrees read it from there.
 */
USTRUCT(meta = (DisplayName = "Find Target", Category = "KzAI|Perception"))
struct KZAI_API FKzStateTreeTask_FindTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FKzStateTreeTask_FindTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
