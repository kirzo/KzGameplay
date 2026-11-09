// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableConditions/ScriptableCondition.h"
#include "Factions/KzFactionSettings.h"
#include "ScriptableCondition_Attitude.generated.h"

/** How one actor sees another through their factions, such as whether what a sensor found is hostile to its owner. */
UCLASS(DisplayName = "Attitude", meta = (ConditionCategory = "Gameplay|Factions"))
class KZGAMEPLAY_API UScriptableCondition_Attitude : public UScriptableCondition
{
	GENERATED_BODY()

protected:
	/** Who looks, such as the sensor's owner. */
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ScriptableContext))
	TObjectPtr<AActor> SourceActor;

	/** Who is looked at, such as what the sensor found. */
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ScriptableContext))
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, Category = "Condition")
	EKzAttitude Attitude = EKzAttitude::Hostile;

	virtual bool Evaluate_Implementation() const override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};
