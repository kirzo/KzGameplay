// Copyright 2026 kirzo

#include "Spawning/KzSpawnVisualizer.h"
#include "Spawning/KzSpawnComponent.h"
#include "Encounter/KzEncounterComponent.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Components/PrimitiveComponent.h"
#include "SceneManagement.h"
#include "UObject/UObjectIterator.h"

void FKzSpawnVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UKzSpawnComponent* Spawn = Cast<UKzSpawnComponent>(Component);
	if (!Spawn)
	{
		return;
	}

	const FLinearColor Color(FColor(245, 165, 36));
	const FTransform Transform = Spawn->GetComponentTransform();

	if (Spawn->SearchRadius > 0.0f)
	{
		DrawCircle(PDI, Transform.GetLocation(), Transform.GetUnitAxis(EAxis::X), Transform.GetUnitAxis(EAxis::Y), Color, Spawn->SearchRadius, 32, SDPG_World, 1.5f);
	}

	if (const UPrimitiveComponent* Collision = UKzSpawnComponent::GetSpawnCollision(Spawn->AutoSpawnClass))
	{
		const FCollisionShape Shape = Collision->GetCollisionShape();
		const FVector Location = Transform.GetLocation() + FVector(0.0f, 0.0f, Shape.GetExtent().Z);
		FKzShapeInstance::MakeFromCollisionShape(Shape).DrawWire(PDI, FTransform(Transform.GetRotation(), Location), Color, 1.5f);
	}

	const AActor* Point = Spawn->GetOwner();
	for (TObjectIterator<UKzEncounterComponent> It; It; ++It)
	{
		const UKzEncounterComponent* Encounter = *It;
		if (Encounter->GetWorld() != Spawn->GetWorld() || Encounter->IsTemplate())
		{
			continue;
		}

		const bool bUses = Encounter->Initial.Contains(Point) || Encounter->Waves.ContainsByPredicate([Point](const FKzEncounterWave& Wave)
		{
			return Wave.Spawns.ContainsByPredicate([Point](const FKzEncounterSpawn& Each) { return Each.SpawnPoint == Point; });
		});
		if (bUses)
		{
			PDI->DrawLine(Transform.GetLocation(), Encounter->GetComponentLocation(), Color, SDPG_World, 2.0f);
		}
	}
}