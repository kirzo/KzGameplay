// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationSource.h"
#include "KzGraphPreview.h"
#include "ScopedTransaction.h"
#include "UObject/GCObject.h"
#include "UObject/SoftObjectPtr.h"

class FKzAnimationPreview;
class UKzComboGraph;
class UKzEdGraphNode;
class UKzItemDefinition;
class UMeshComponent;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
struct FKzComboHitbox;
struct FKzComboNode_Attack;

/**
 * The combo editor's viewport: the selected attack's animation on the combo's preview character, holding the preview item, with the attack's hitboxes
 * and the trail they sweep through its Hit Windows. Clicking a hitbox puts a gizmo on it to move and turn it. A transport bar under it plays, pauses,
 * steps and scrubs, with the windows as bands.
 */
class FKzComboPreview : public FKzGraphPreview, public FGCObject
{
public:
	explicit FKzComboPreview(UKzComboGraph& InCombo) : Combo(&InCombo) {}

	virtual void Initialize(FAdvancedPreviewScene& InScene) override;
	virtual void OnSelectionChanged(const TSet<UObject*>& Selection) override;
	virtual void OnGraphChanged() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas) override;
	virtual FBox GetBounds() const override;
	virtual TSharedPtr<SWidget> MakeBottomPanel() override;
	virtual bool HandleClick(HHitProxy* HitProxy) override;
	virtual TOptional<FTransform> GetGizmoTransform() const override;
	virtual void MoveGizmo(const FVector& Drag, const FRotator& Rotation) override;
	virtual void BeginGizmoDrag() override;
	virtual void EndGizmoDrag() override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FKzComboPreview"); }

	float GetLength() const;
	float GetTime() const;
	bool IsPlaying() const;
	void TogglePlay();

	/** Pauses and poses the animation at Time, as dragging a playhead does. */
	void ScrubTo(float Time);

	const TArray<FKzAnimationNotifySpan>& GetNotifies() const { return Notifies; }

private:
	/** A hitbox where it was at a time of the animation. */
	struct FTrailSample
	{
		float Time = 0.0f;
		FTransform Transform;
	};

	/** Puts the character the combo's settings name in the scene. */
	void ShowCharacter();

	/** Hangs the preview item on the character as equipping it would. */
	void ShowItem();

	/** Plays the selected attack's animation on the character, keeping the playhead where it was. */
	void ShowAnimation();

	const FKzComboNode_Attack* GetSelectedAttack() const;
	const USceneComponent* GetHitboxParent(const FKzComboHitbox& Hitbox) const;
	TOptional<FTransform> GetHitboxTransform(const FKzComboHitbox& Hitbox) const;
	bool IsInHitWindow(float Time) const;

	/** Remembers where the hitboxes are while a Hit Window plays, one sample per frame of the animation. */
	void RecordTrails();

	TSharedRef<SWidget> MakeTransportBar();

	TWeakObjectPtr<UKzComboGraph> Combo;
	FAdvancedPreviewScene* Scene = nullptr;
	TObjectPtr<AActor> Character;
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** The preview item, as an actor or as a mesh, as its Equippable fragment spawns it. */
	TObjectPtr<AActor> Weapon;
	TObjectPtr<UMeshComponent> WeaponMesh;

	TWeakObjectPtr<UKzEdGraphNode> SelectedNode;
	TSharedPtr<FKzAnimationPreview> Animation;

	TArray<FKzAnimationNotifySpan> Notifies;

	/** The hitbox of the selected attack the gizmo is on. */
	int32 SelectedHitbox = INDEX_NONE;
	TUniquePtr<FScopedTransaction> GizmoTransaction;

	/** Per hitbox of the selected attack, where it went through the Hit Windows played so far. */
	TArray<TArray<FTrailSample>> Trails;

	bool bLooping = true;
	float Rate = 1.0f;

	/** The playhead on the last tick, to tell when it went round. */
	float LastTime = 0.0f;

	/** The settings the scene was made from, to make it again when they change. */
	TSoftClassPtr<AActor> ShownCharacter;
	TSoftObjectPtr<USkeletalMesh> ShownMesh;
	TSoftObjectPtr<UKzItemDefinition> ShownItem;
};