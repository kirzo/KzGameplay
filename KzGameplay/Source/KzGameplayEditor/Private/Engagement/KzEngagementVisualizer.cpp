// Copyright 2026 kirzo

#include "Engagement/KzEngagementVisualizer.h"
#include "Engagement/KzEngagementComponent.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "SceneManagement.h"
#include "SceneView.h"

void FKzEngagementVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UKzEngagementComponent* Engagement = Cast<UKzEngagementComponent>(Component))
	{
		Engagement->ForEachDebugLine([PDI](const FVector& Start, const FVector& End, const FLinearColor& Color)
		{
			PDI->DrawLine(Start, End, Color, SDPG_World, 1.5f);
		});
	}
}

void FKzEngagementVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	const UKzEngagementComponent* Engagement = Cast<UKzEngagementComponent>(Component);
	if (!Engagement || !Canvas)
	{
		return;
	}

	const float DPIScale = Canvas->GetDPIScale();
	Engagement->ForEachDebugLabel(false, [View, Canvas, DPIScale](const FVector& Location, const FString& Text, const FLinearColor& Color)
	{
		FVector2D Pixel;
		if (View->WorldToPixel(Location, Pixel))
		{
			Canvas->DrawShadowedString(Pixel.X / DPIScale, Pixel.Y / DPIScale, *Text, GEngine->GetSmallFont(), Color);
		}
	});
}