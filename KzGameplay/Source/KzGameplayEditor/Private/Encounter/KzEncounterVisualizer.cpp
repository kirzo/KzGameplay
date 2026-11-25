// Copyright 2026 kirzo

#include "Encounter/KzEncounterVisualizer.h"
#include "Encounter/KzEncounterComponent.h"
#include "SceneManagement.h"

void FKzEncounterVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UKzEncounterComponent* Encounter = Cast<UKzEncounterComponent>(Component);
	if (!Encounter)
	{
		return;
	}

	const FVector Start = Encounter->GetComponentLocation();
	const FLinearColor InitialColor(FColor(245, 165, 36));
	const FLinearColor WaveColor(FColor(255, 214, 120));
	const FLinearColor LimitColor(FColor(230, 70, 60));

	for (const AActor* Actor : Encounter->Initial)
	{
		if (Actor)
		{
			PDI->DrawLine(Start, Actor->GetActorLocation(), InitialColor, SDPG_World, 2.0f);
		}
	}

	TSet<const AActor*> Points;
	for (const FKzEncounterWave& Wave : Encounter->Waves)
	{
		for (const FKzEncounterSpawn& Spawn : Wave.Spawns)
		{
			if (Spawn.SpawnPoint && !Points.Contains(Spawn.SpawnPoint))
			{
				Points.Add(Spawn.SpawnPoint);
				PDI->DrawLine(Start, Spawn.SpawnPoint->GetActorLocation(), WaveColor, SDPG_World, 2.0f);
			}
		}
	}

	for (const AActor* Limit : Encounter->Limits)
	{
		if (Limit)
		{
			DrawDashedLine(PDI, Start, Limit->GetActorLocation(), LimitColor, 40.0f, SDPG_World);
		}
	}
}