// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_EquipItem.generated.h"

class UKzItemDefinition;

/**
 * Task that automatically equips an item on the target actor.
 * Fails silently if the actor does not have a UKzEquipmentComponent.
 */
UCLASS(DisplayName = "Equip Item", meta = (TaskCategory = "Gameplay|Equipment"))
class KZGAMEPLAY_API UScriptableTask_EquipItem : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The actor that will receive the item. */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	TObjectPtr<AActor> TargetActor;

	/** The item definition to equip. */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	TSoftObjectPtr<UKzItemDefinition> ItemToEquip;

	/** If true, calling Reset() on this task will unequip the item that was equipped. */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	bool bRevertOnReset = false;

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};