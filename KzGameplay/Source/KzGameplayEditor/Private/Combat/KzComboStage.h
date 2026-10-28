// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

class AActor;
class FPreviewScene;
class FReferenceCollector;
class UKzComboGraph;
class UKzItemDefinition;
class UMeshComponent;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/** The combo's preview character holding its preview item, put in a preview scene as the game would. Shared by the viewport and the previews in the nodes. */
class FKzComboStage
{
public:
	/** Puts in the scene what the combo's preview settings name, if they changed since last time. True when it changed anything. */
	bool Update(FPreviewScene& Scene, const UKzComboGraph& Combo);

	/** Where the previews look at the character from: its front and a side, a little from above. */
	static FVector GetViewDirection() { return FVector(1.0, 0.6, 0.3).GetSafeNormal(); }

	AActor* GetCharacter() const { return Character; }
	USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/** What weapon hitboxes hang from: the item's actor root, or its mesh. */
	USceneComponent* GetWeaponRoot() const;

	void AddReferencedObjects(FReferenceCollector& Collector);

private:
	void ShowCharacter(FPreviewScene& Scene, const UKzComboGraph& Combo);

	/** Hangs the preview item on the character as equipping it would. */
	void ShowItem(FPreviewScene& Scene, const UKzComboGraph& Combo);

	TObjectPtr<AActor> Character;
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** The preview item, as an actor or as a mesh, as its Equippable fragment spawns it. */
	TObjectPtr<AActor> Weapon;
	TObjectPtr<UMeshComponent> WeaponMesh;

	/** The settings the scene was made from. */
	bool bShown = false;
	TSoftClassPtr<AActor> ShownCharacter;
	TSoftObjectPtr<USkeletalMesh> ShownMesh;
	TSoftObjectPtr<UKzItemDefinition> ShownItem;
};