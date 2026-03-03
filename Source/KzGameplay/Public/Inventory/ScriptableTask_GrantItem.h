// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_GrantItem.generated.h"

class UKzItemDefinition;

/**
 * Task that grants a specific quantity of an item directly to the target's inventory.
 * Fails silently if the actor does not have a UKzInventoryComponent.
 */
UCLASS(DisplayName = "Grant Item", meta = (TaskCategory = "Gameplay|Inventory"))
class KZGAMEPLAY_API UScriptableTask_GrantItem : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The actor that will receive the item. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TObjectPtr<AActor> TargetActor;

	/** The item definition to add to the inventory. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSoftObjectPtr<UKzItemDefinition> ItemToGrant;

	/** Number of items to grant. */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** If true, calling Reset() on this task will remove the item that was granted. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	bool bRevertOnReset = false;

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};