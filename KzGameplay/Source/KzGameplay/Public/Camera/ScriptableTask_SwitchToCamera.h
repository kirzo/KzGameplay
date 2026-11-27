// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_SwitchToCamera.generated.h"

/**
 * Task that shows a camera through the camera director, blending from the current view, such as an arena's camera.
 * A camera with a camera rig becomes the gameplay camera; one without is a fixed shot.
 */
UCLASS(DisplayName = "Switch To Camera", meta = (TaskCategory = "Gameplay|Camera"))
class KZGAMEPLAY_API UScriptableTask_SwitchToCamera : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The camera to show, such as one placed in the level. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	TObjectPtr<AActor> Camera;

	/** How it blends in, and back on reset. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	FViewTargetTransitionParams Blend;

	/** If true, calling Reset() on this task goes back to the gameplay camera there was, unless the view already left this camera. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bRevertOnReset = false;

	virtual bool IsStoppable() const override { return false; }

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

private:
	bool bSwitched = false;
	TWeakObjectPtr<AActor> PreviousGameplayCamera;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};