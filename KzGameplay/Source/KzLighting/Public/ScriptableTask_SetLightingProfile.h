// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "KzLightingProfile.h"
#include "ScriptableTask_SetLightingProfile.generated.h"

class AKzLightingRig;

/**
 * Task that lights the level with a profile, as a layer on the lighting rig. The layer lasts while the action that ran it does.
 * Does nothing without a lighting rig in the world.
 */
UCLASS(DisplayName = "Set Lighting Profile", meta = (TaskCategory = "Gameplay|Lighting"))
class KZLIGHTING_API UScriptableTask_SetLightingProfile : public UScriptableTask
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ShowOnlyInnerProperties))
	FKzLightingLayerSettings Layer;

	/** Skips the blend in, and the blend out on reset. */
	UPROPERTY(EditAnywhere, Category = "Lighting")
	bool bSnap = false;

	/** If true, calling Reset() on this task takes the profile away, blending back to what is under it. */
	UPROPERTY(EditAnywhere, Category = "Lighting")
	bool bRevertOnReset = false;

	virtual bool IsStoppable() const override { return false; }

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

private:
	TWeakObjectPtr<AKzLightingRig> PushedTo;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};