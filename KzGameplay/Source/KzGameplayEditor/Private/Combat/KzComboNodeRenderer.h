// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzComboStage.h"
#include "TickableEditorObject.h"
#include "UObject/GCObject.h"

class FAdvancedPreviewScene;
class FKzAnimationPreview;
class UKzComboGraph;
class UKzEdGraphNode;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Renders the attacks shown inside a combo's nodes: a small scene of its own with the combo's preview character and item, posed by each attack
 * in turn and captured into the node's texture. Shared by the combo's nodes and its editor while any of them holds it.
 */
class FKzComboNodeRenderer : public FGCObject, public FTickableEditorObject
{
public:
	static constexpr int32 Width = 192;
	static constexpr int32 Height = 108;

	/** The combo's renderer, made the first time it is asked for. */
	static TSharedRef<FKzComboNodeRenderer> Get(UKzComboGraph& Combo);

	explicit FKzComboNodeRenderer(UKzComboGraph& InCombo);
	virtual ~FKzComboNodeRenderer() override;

	/** Renders the node's attack into Target on the next editor tick: at Time, looping, or unset at the moment that shows it best. */
	void Request(const UKzEdGraphNode& Node, TOptional<float> Time, UTextureRenderTarget2D& Target);

	/** Forgets what the node played, after it changed. */
	void Invalidate(const UKzEdGraphNode& Node);

	/** The nodes selected in the combo's editor, which play live as a hovered one does. */
	void SetSelection(const TSet<UObject*>& Selection);
	bool IsSelected(const UKzEdGraphNode& Node) const;

	/** The attack the editor's viewport is playing in a whole combo, which its node marks; none outside one. */
	void SetPlayingNode(const UKzEdGraphNode* Node);
	bool IsPlaying(const UKzEdGraphNode& Node) const;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FKzComboNodeRenderer"); }

	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override;

private:
	struct FRequest
	{
		TWeakObjectPtr<const UKzEdGraphNode> Node;
		TOptional<float> Time;
	};

	/** Poses the node's attack on the stage; false when it has nothing to show. */
	bool Prepare(const UKzEdGraphNode& Node);

	/** Puts the cameras on the character, seen from the front and a side. */
	void Frame();

	/** Target's own camera, which keeps its own rendering history so the frames it settles over are its own. */
	USceneCaptureComponent2D& GetCapture(UTextureRenderTarget2D& Target);

	TWeakObjectPtr<UKzComboGraph> Combo;
	TUniquePtr<FAdvancedPreviewScene> Scene;
	FKzComboStage Stage;
	TMap<TWeakObjectPtr<UTextureRenderTarget2D>, TObjectPtr<USceneCaptureComponent2D>> Captures;
	FTransform CameraTransform;

	TWeakObjectPtr<const UKzEdGraphNode> PreparedNode;
	TSharedPtr<FKzAnimationPreview> Animation;
	TArray<FKzAnimationNotifySpan> Notifies;

	TMap<TWeakObjectPtr<UTextureRenderTarget2D>, FRequest> Requests;
	TSet<TWeakObjectPtr<const UObject>> Selected;
	TWeakObjectPtr<const UKzEdGraphNode> PlayingNode;
};