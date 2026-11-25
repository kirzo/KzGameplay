// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

/** Draws lines from a selected encounter to what it uses: its Initial enemies and points, the spawn points of its waves, and its Limits dashed. */
class FKzEncounterVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};