// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

/** Draws a selected engagement component's areas and sectors on the ground, with each sector's weight and each area's capacity. */
class FKzEngagementVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
};