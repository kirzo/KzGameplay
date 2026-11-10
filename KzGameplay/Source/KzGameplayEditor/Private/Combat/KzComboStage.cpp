// Copyright 2026 kirzo

#include "Combat/KzComboStage.h"
#include "Combat/KzComboGraph.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Equipment/KzEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Items/KzItemDefinition.h"
#include "PreviewScene.h"
#include "UObject/GCObject.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	// Collision shapes, arrows and the like get in the way of the attack.
	void ShowMeshesOnly(AActor& Actor)
	{
		TInlineComponentArray<UPrimitiveComponent*> Primitives(&Actor);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (!Primitive->IsA<UMeshComponent>())
			{
				Primitive->SetVisibility(false);
			}
		}
	}
}

FKzComboStage::FKzComboStage()
{
	ObjectsReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FKzComboStage::OnObjectsReplaced);
	if (GEditor)
	{
		BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(this, &FKzComboStage::OnBlueprintCompiled);
	}
}

FKzComboStage::~FKzComboStage()
{
	FCoreUObjectDelegates::OnObjectsReplaced.Remove(ObjectsReplacedHandle);
	if (GEditor)
	{
		GEditor->OnBlueprintCompiled().Remove(BlueprintCompiledHandle);
	}
}

void FKzComboStage::OnObjectsReplaced(const TMap<UObject*, UObject*>& Replaced)
{
	for (const TPair<UObject*, UObject*>& Pair : Replaced)
	{
		// The engine may have handed the stage its new instances already.
		for (TObjectPtr<AActor>* Actor : { &Character, &Weapon })
		{
			if (*Actor && (Pair.Key == Actor->Get() || Pair.Value == Actor->Get()))
			{
				*Actor = Cast<AActor>(Pair.Value);
				bShown = false;
			}
		}
	}
}

bool FKzComboStage::Update(FPreviewScene& Scene, const UKzComboGraph& Combo)
{
	const bool bNewCharacter = !bShown || Combo.PreviewCharacter != ShownCharacter || Combo.PreviewMesh != ShownMesh;
	const bool bNewItem = bNewCharacter || Combo.PreviewItem != ShownItem;
	if (bNewCharacter)
	{
		ShowCharacter(Scene, Combo);
	}
	if (bNewItem)
	{
		ShowItem(Scene, Combo);
	}
	bShown = true;
	return bNewItem;
}

USceneComponent* FKzComboStage::GetWeaponRoot() const
{
	return Weapon ? Weapon->GetRootComponent() : WeaponMesh.Get();
}

void FKzComboStage::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Character);
	Collector.AddReferencedObject(Mesh);
	Collector.AddReferencedObject(Weapon);
	Collector.AddReferencedObject(WeaponMesh);
}

void FKzComboStage::ShowCharacter(FPreviewScene& Scene, const UKzComboGraph& Combo)
{
	if (Character)
	{
		Character->Destroy();
	}
	else if (Mesh)
	{
		Scene.RemoveComponent(Mesh);
	}
	Character = nullptr;
	Mesh = nullptr;

	ShownCharacter = Combo.PreviewCharacter;
	ShownMesh = Combo.PreviewMesh;

	if (UClass* Class = ShownCharacter.LoadSynchronous())
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags = RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character = Scene.GetWorld()->SpawnActor<AActor>(Class, FTransform::Identity, Params);
		const ACharacter* AsCharacter = Cast<ACharacter>(Character);
		Mesh = AsCharacter ? AsCharacter->GetMesh() : nullptr;
		if (Character)
		{
			ShowMeshesOnly(*Character);
		}
	}

	USkeletalMesh* MeshAsset = ShownMesh.LoadSynchronous();
	if (MeshAsset && !Mesh)
	{
		Mesh = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
		Scene.AddComponent(Mesh, FTransform::Identity);
	}
	if (!Mesh)
	{
		return;
	}

	if (MeshAsset)
	{
		Mesh->SetSkeletalMeshAsset(MeshAsset);
	}

	// Feet on the floor: a character's mesh hangs below its capsule's middle.
	if (Character)
	{
		Character->SetActorLocation(FVector(0.0, 0.0, -Mesh->GetRelativeLocation().Z));
	}

	// Its own AnimBlueprint gives way to the attacks played on it.
	Mesh->SetUpdateAnimationInEditor(true);
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void FKzComboStage::ShowItem(FPreviewScene& Scene, const UKzComboGraph& Combo)
{
	if (Weapon)
	{
		Weapon->Destroy();
	}
	if (WeaponMesh)
	{
		Scene.RemoveComponent(WeaponMesh);
	}
	Weapon = nullptr;
	WeaponMesh = nullptr;

	ShownItem = Combo.PreviewItem;
	const UKzItemDefinition* Item = ShownItem.LoadSynchronous();
	const UKzItemFragment_Equippable* Equippable = Item ? Item->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
	if (!Mesh || !Equippable)
	{
		return;
	}

	// Where the character's equipment would hang it; without equipment, from the mesh by the item's own socket.
	FName Socket = Equippable->SocketOverride;
	const UKzEquipmentComponent* Equipment = Character ? Character->FindComponentByClass<UKzEquipmentComponent>() : nullptr;
	USceneComponent* Parent = Equipment ? Equipment->FindAttachmentForItem(*Equippable, Socket) : nullptr;
	if (!Parent)
	{
		Parent = Mesh;
	}

	if (Equippable->EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh)
	{
		UStreamableRenderAsset* MeshAsset = Equippable->EquipmentMesh.LoadSynchronous();
		if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshAsset))
		{
			UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			Component->SetStaticMesh(StaticMesh);
			WeaponMesh = Component;
		}
		else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(MeshAsset))
		{
			USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			Component->SetSkeletalMeshAsset(SkeletalMesh);
			WeaponMesh = Component;
		}

		if (WeaponMesh)
		{
			Scene.AddComponent(WeaponMesh, FTransform::Identity);
			WeaponMesh->AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
			WeaponMesh->SetRelativeTransform(Equippable->AttachmentOffset);
		}
		return;
	}

	if (UClass* Class = Equippable->GetEquippedActorClass(Item->WorldActorClass).LoadSynchronous())
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags = RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Weapon = Scene.GetWorld()->SpawnActor<AActor>(Class, FTransform::Identity, Params);
		if (Weapon)
		{
			Weapon->SetActorEnableCollision(false);
			if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Weapon->GetRootComponent()))
			{
				Root->SetSimulatePhysics(false);
			}
			Weapon->AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
			Weapon->SetActorRelativeTransform(Equippable->AttachmentOffset);
			ShowMeshesOnly(*Weapon);
		}
	}
}