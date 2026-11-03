// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_StowItem.generated.h"

/** Puts the item in a slot away on the target actor, or draws it back, at once. To do it with an animation, play one with a Stow Item notify. */
UCLASS(DisplayName = "Stow Item", meta = (TaskCategory = "Gameplay|Equipment"))
class KZGAMEPLAY_API UScriptableTask_StowItem : public UScriptableTask
{
	GENERATED_BODY()

public:
	UScriptableTask_StowItem();

	UPROPERTY(EditAnywhere, Category = "Equipment")
	TObjectPtr<AActor> TargetActor;

	/** The slot the item is used from, such as the hand. */
	UPROPERTY(EditAnywhere, Category = "Equipment", meta = (Categories = "Equipment.Slot"))
	FGameplayTag Slot;

	/** Draws the item back into Slot instead of putting it away. */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	bool bDraw = false;

	/** Undoes it on Reset(): draws back what it put away, or puts away what it drew. */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	bool bRevertOnReset = false;

	virtual bool IsStoppable() const { return false; }

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

private:
	void Apply(bool bDrawItem) const;
};
