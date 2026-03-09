// Copyright 2026 kirzo

#include "Items/Fragments/KzItemFragment_MeleeWeapon.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

bool UKzItemFragment_MeleeWeapon::GetComboStep(int32 Index, FKzMeleeComboStep& OutStep) const
{
	if (ComboSteps.IsValidIndex(Index))
	{
		OutStep = ComboSteps[Index];
		return true;
	}
	return false;
}

float UKzItemFragment_MeleeWeapon::CalculateDamageForStep(int32 Index) const
{
	if (ComboSteps.IsValidIndex(Index))
	{
		return BaseDamage * ComboSteps[Index].DamageMultiplier;
	}
	return BaseDamage;
}

int32 UKzItemFragment_MeleeWeapon::GetNextComboIndex(int32 CurrentIndex, bool bLoop) const
{
	if (ComboSteps.IsEmpty())
	{
		return INDEX_NONE;
	}

	int32 NextIndex = CurrentIndex + 1;

	if (NextIndex >= ComboSteps.Num())
	{
		return bLoop ? 0 : (ComboSteps.Num() - 1);
	}

	return NextIndex;
}

bool UKzItemFragment_MeleeWeapon::GetCollisionDataForStep(int32 Index, AActor* AvatarActor, AActor* WeaponActor, FKzShapeInstance& OutShape, FKzTransformSource& OutTransformSource) const
{
	if (!ComboSteps.IsValidIndex(Index))
	{
		return false;
	}

	const FKzMeleeComboStep& Step = ComboSteps[Index];

	// 1. Output the exact shape defined by the designers
	OutShape = Step.TraceShape;

	// 2. Resolve the target mesh based on the step configuration
	UMeshComponent* TargetMesh = nullptr;

	if (Step.MeshTarget == EKzMeleeMeshTarget::Weapon && WeaponActor)
	{
		// Try to find a specific tagged mesh ("MeleeMesh"), otherwise fallback to any MeshComponent
		TArray<UMeshComponent*> Meshes;
		WeaponActor->GetComponents<UMeshComponent>(Meshes);

		for (UMeshComponent* Mesh : Meshes)
		{
			if (Mesh->ComponentHasTag(TEXT("MeleeMesh")))
			{
				TargetMesh = Mesh;
				break;
			}
		}

		// If no specific tag was found, just grab the first mesh available
		if (!TargetMesh && Meshes.Num() > 0)
		{
			TargetMesh = Meshes[0];
		}
	}
	else if (Step.MeshTarget == EKzMeleeMeshTarget::Avatar && AvatarActor)
	{
		// Unarmed combat: grab the character's primary skeletal mesh
		TargetMesh = AvatarActor->FindComponentByClass<USkeletalMeshComponent>();
	}

	// 3. Build the generic transform source safely
	if (TargetMesh)
	{
		OutTransformSource.Initialize(TargetMesh, Step.TraceSocketName, Step.TraceOffset);
		return true;
	}

	return false;
}