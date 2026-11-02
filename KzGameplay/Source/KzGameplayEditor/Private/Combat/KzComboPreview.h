// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzComboStage.h"
#include "KzGraphPreview.h"
#include "ScopedTransaction.h"
#include "UObject/GCObject.h"

class FKzAnimationPreview;
class FKzComboNodeRenderer;
class UKzComboGraph;
class UKzEdGraphNode;
class USceneComponent;
struct FKzComboHitbox;
struct FKzComboNode_Attack;

/**
 * The combo editor's viewport: the selected attack's animation on the combo's preview character, holding the preview item, with the attack's hitboxes
 * and the trail they sweep through its Hit Windows. In combo mode it plays the attacks from the Entry up to the selected one instead, chaining each at
 * the combo's preview timing. Clicking a hitbox puts a gizmo on it to move and turn it. A transport bar under it plays, pauses, steps and scrubs,
 * with the windows as bands.
 */
class FKzComboPreview : public FKzGraphPreview, public FGCObject
{
public:
	/** One attack of what plays, laid on the preview's timeline, in animation time. */
	struct FStep
	{
		TWeakObjectPtr<UKzEdGraphNode> Node;
		float Start = 0.0f;

		/** When the next attack takes over, or this one ends for the last. */
		float End = 0.0f;
		float Length = 0.0f;
		TArray<FKzAnimationNotifySpan> Notifies;

		/** Chained at its end for want of a Combo Window, which in game would stop the combo there. */
		bool bNoComboWindow = false;
	};

	explicit FKzComboPreview(UKzComboGraph& InCombo) : Combo(&InCombo) {}

	virtual void Initialize(FAdvancedPreviewScene& InScene) override;
	virtual void OnSelectionChanged(const TSet<UObject*>& Selection) override;
	virtual void OnGraphChanged() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas) override;
	virtual FBox GetBounds() const override;
	virtual TOptional<FRotator> GetViewRotation() const override { return (-FKzComboStage::GetViewDirection()).Rotation(); }
	virtual TSharedPtr<SWidget> MakeBottomPanel() override;
	virtual bool HandleClick(HHitProxy* HitProxy) override;
	virtual TOptional<FTransform> GetGizmoTransform() const override;
	virtual void MoveGizmo(const FVector& Drag, const FRotator& Rotation) override;
	virtual void BeginGizmoDrag() override;
	virtual void EndGizmoDrag() override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FKzComboPreview"); }

	float GetLength() const;
	float GetTime() const { return Time; }
	bool IsPlaying() const { return bPlaying; }
	virtual bool CanTogglePlay() const override { return !Steps.IsEmpty(); }
	virtual void TogglePlay() override;

	/** Pauses and poses the attacks at Time, as dragging a playhead does. */
	void ScrubTo(float InTime);

	const TArray<FStep>& GetSteps() const { return Steps; }

private:
	/** A way from the Entry to the selected attack, and the outputs it takes. */
	struct FPath
	{
		TArray<TWeakObjectPtr<UKzEdGraphNode>> Nodes;
		FText Label;
	};

	/** A hitbox where it was at a time of the animation. */
	struct FTrailSample
	{
		float Time = 0.0f;
		FTransform Transform;
	};

	/** Lays out what plays: the selected attack, or in combo mode the attacks of the chosen way to it. Keeps the playhead where it was. */
	void BuildSteps();

	/** Every way from the Entry to the selected attack that passes no attack twice, shortest first. */
	void FindPaths();

	/** Poses the character at Time, putting on the animation of the attack playing then. */
	void PoseAt(float InTime);

	FKzAnimationContext MakeContext() const;
	const FStep* GetCurrentStep() const;
	UKzEdGraphNode* GetShownNode() const;
	const FKzComboNode_Attack* GetShownAttack() const;
	const USceneComponent* GetHitboxParent(const FKzComboHitbox& Hitbox) const;
	TOptional<FTransform> GetHitboxTransform(const FKzComboHitbox& Hitbox) const;

	/** Whether the attack playing is inside a Hit Window at its own time LocalTime. */
	bool IsInHitWindow(float LocalTime) const;

	/** Remembers where the hitboxes are while a Hit Window plays, one sample per frame of the animation. */
	void RecordTrails();

	TSharedRef<SWidget> MakeTransportBar();

	TWeakObjectPtr<UKzComboGraph> Combo;
	FAdvancedPreviewScene* Scene = nullptr;
	FKzComboStage Stage;

	/** Renders the previews in the combo's nodes, kept while the editor is open so they know the selection and the attack playing. */
	TSharedPtr<FKzComboNodeRenderer> NodeRenderer;

	TWeakObjectPtr<UKzEdGraphNode> SelectedNode;

	/** Whether it plays the way to the selected attack rather than the attack alone, and which way. */
	bool bComboMode = false;
	TArray<FPath> Paths;
	int32 PathIndex = 0;

	TArray<FStep> Steps;
	int32 CurrentStep = INDEX_NONE;

	/** The animation of the attack playing, posed at each time rather than left to play, so the preview runs the whole timeline. */
	TSharedPtr<FKzAnimationPreview> Animation;

	/** The attack before it, which in game goes on underneath while this one blends in. */
	TSharedPtr<FKzAnimationPreview> PreviousAnimation;

	float Time = 0.0f;
	bool bPlaying = true;
	bool bLooping = true;
	float Rate = 1.0f;

	/** The hitbox of the attack playing the gizmo is on. */
	int32 SelectedHitbox = INDEX_NONE;
	TUniquePtr<FScopedTransaction> GizmoTransaction;

	/** Per hitbox of the attack playing, where it went through the Hit Windows played so far. */
	TArray<TArray<FTrailSample>> Trails;
};