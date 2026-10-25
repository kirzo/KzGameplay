// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationSource.h"
#include "KzGraphPreview.h"
#include "UObject/GCObject.h"
#include "UObject/SoftObjectPtr.h"

class FKzAnimationPreview;
class UKzComboGraph;
class UKzEdGraphNode;
class USkeletalMesh;
class USkeletalMeshComponent;
struct FKzComboNode_Attack;

/** The combo editor's viewport: the selected attack's animation on the combo's preview character, posed by a timeline with its windows. */
class FKzComboPreview : public FKzGraphPreview, public FGCObject
{
public:
	explicit FKzComboPreview(UKzComboGraph& InCombo) : Combo(&InCombo) {}

	virtual void Initialize(FAdvancedPreviewScene& InScene) override;
	virtual void OnSelectionChanged(const TSet<UObject*>& Selection) override;
	virtual void OnGraphChanged() override;
	virtual void DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas) override;
	virtual FBox GetBounds() const override;
	virtual TSharedPtr<SWidget> MakeBottomPanel() override;

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
	/** Puts the character the combo's settings name in the scene. */
	void ShowCharacter();

	/** Plays the selected attack's animation on the character, keeping the playhead where it was. */
	void ShowAnimation();

	const FKzComboNode_Attack* GetSelectedAttack() const;

	TWeakObjectPtr<UKzComboGraph> Combo;
	FAdvancedPreviewScene* Scene = nullptr;
	TObjectPtr<AActor> Character;
	TObjectPtr<USkeletalMeshComponent> Mesh;
	TWeakObjectPtr<UKzEdGraphNode> SelectedNode;
	TSharedPtr<FKzAnimationPreview> Animation;
	TArray<FKzAnimationNotifySpan> Notifies;

	/** The settings the character was made from, to make it again when they change. */
	TSoftClassPtr<AActor> ShownCharacter;
	TSoftObjectPtr<USkeletalMesh> ShownMesh;
};