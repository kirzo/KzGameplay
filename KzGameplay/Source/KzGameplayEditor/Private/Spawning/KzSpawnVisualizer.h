// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

/** Draws a selected spawn component's Search Radius on the ground and, with an Auto Spawn Class, where its collision goes. */
class FKzSpawnVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};