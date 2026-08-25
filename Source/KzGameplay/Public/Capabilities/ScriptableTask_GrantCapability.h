// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "GameplayTagContainer.h"
#include "ScriptableTask_GrantCapability.generated.h"

class UKzCapabilitySet;

/**
 * Task that grants a capability to the target, along with whatever implements it.
 * Fails silently if the actor has no UKzCapabilityComponent.
 */
UCLASS(DisplayName = "Grant Capability", meta = (TaskCategory = "Gameplay|Capabilities"))
class KZGAMEPLAY_API UScriptableTask_GrantCapability : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The actor that will receive the capability. */
	UPROPERTY(EditAnywhere, Category = "Capability", meta = (ScriptableContext))
	TObjectPtr<AActor> TargetActor;

	/** The capability to grant. */
	UPROPERTY(EditAnywhere, Category = "Capability", meta = (Categories = "Capability"))
	FGameplayTag Capability;

	/**
	 * Where to look up what implements it. Leave empty to use the target's own set, or point it at another
	 * set to lend a capability with your implementation rather than theirs.
	 */
	UPROPERTY(EditAnywhere, Category = "Capability")
	TObjectPtr<UKzCapabilitySet> SourceSet;

	/** If true, calling Reset() on this task will revoke the capability that was granted. */
	UPROPERTY(EditAnywhere, Category = "Capability")
	bool bRevertOnReset = false;

	virtual bool IsStoppable() const { return false; }

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

private:
	/** Whether we were the ones who granted it, so Reset does not revoke something that was already there. */
	UPROPERTY(Transient)
	bool bGranted = false;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};
