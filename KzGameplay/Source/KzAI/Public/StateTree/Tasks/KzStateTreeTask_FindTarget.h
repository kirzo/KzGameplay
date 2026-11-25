// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "Factions/KzFactionSettings.h"
#include "KzStateTreeTask_FindTarget.generated.h"

class UKzSensorComponent;

USTRUCT()
struct FKzStateTreeTask_FindTargetInstanceData
{
	GENERATED_BODY()

	FKzStateTreeTask_FindTargetInstanceData();

	/** The pawn whose attitude counts. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** Where the candidates come from: the pawn's Sensor.Perception sensor. Its range and requirements are what the AI perceives. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<UKzSensorComponent> Sensor = nullptr;

	/** How the pawn must see a candidate to pick it. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	EKzAttitude Attitude = EKzAttitude::Hostile;

	/** Gameplay events whose Instigator it takes while the sensor has nobody, such as being picked by an attack or hit by one: they reveal whoever is out of sight. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagContainer RevealEvents;

	/** The nearest candidate, kept while the sensor still has it so the AI doesn't switch back and forth. None without one. */
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> Target = nullptr;

	/** The Instigator of the last of the Reveal Events, for while the sensor has nobody. */
	UPROPERTY()
	TWeakObjectPtr<AActor> Revealed;

	/** The last AI.Target.Suggested, for while the sensor has nobody and nothing revealed anyone. */
	UPROPERTY()
	TWeakObjectPtr<AActor> Suggested;
};

/**
 * Picks a target among what a sensor perceives, by the pawn's attitude towards it, so a tree speaks of hostiles or allies
 * instead of characters or factions. Meant as a global task: it keeps Target up to date while the tree runs. Promoted to a
 * parameter, the other nodes and linked subtrees read it from there.
 * It only works on events, such as the AI.Sense.Begin and AI.Sense.End the controller sends, and sends AI.Target.Changed when
 * Target changes, so transitions wait on it instead of checking every tick. While the sensor has nobody, it takes whoever the last of its
 * Reveal Events came from, such as an attacker out of sight, or else the target of the last AI.Target.Suggested gameplay event, such
 * as the one an encounter sends to the enemies it spawns.
 */
USTRUCT(meta = (DisplayName = "Find Target", Category = "KzAI|Perception"))
struct KZAI_API FKzStateTreeTask_FindTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FKzStateTreeTask_FindTarget()
	{
		bShouldCallTick = false;
		bShouldCallTickOnlyOnEvents = true;
	}

	using FInstanceDataType = FKzStateTreeTask_FindTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
