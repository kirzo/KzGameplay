// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

/**
 * What the cameras are doing: the camera in view and why it was chosen, the setup of the one in view (or blending in) with who gave
 * each layer, and the latest transitions. The rig in view draws its debug, and the camera zones show with an arrow to their camera.
 */
class FKzCameraDebuggerCategory : public FGameplayDebuggerCategory
{
public:
	FKzCameraDebuggerCategory();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;
};
#endif