// Copyright 2026 kirzo

#include "Combat/KzComboPreview.h"
#include "AdvancedPreviewScene.h"
#include "Algo/BinarySearch.h"
#include "Animation/KzAnimationNotifyState.h"
#include "Animation/KzAnimationPreview.h"
#include "CanvasTypes.h"
#include "Combat/KzCombatNotifies.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboInstance.h"
#include "Combat/KzComboNodeRenderer.h"
#include "Combat/KzComboNodes.h"
#include "Components/SkeletalMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Engine.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HitProxies.h"
#include "Items/KzItemDefinition.h"
#include "Kismet/KzSystemLibrary.h"
#include "KzEdGraphNode.h"
#include "KzGraphEditorUtils.h"
#include "SceneManagement.h"
#include "Sensors/KzSensorComponent.h"
#include "Sensors/KzSensorTags.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KzComboPreview"

struct HKzComboHitboxProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	explicit HKzComboHitboxProxy(int32 InIndex) : HHitProxy(HPP_UI), Index(InIndex) {}

	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::Crosshairs; }

	int32 Index = INDEX_NONE;
};

IMPLEMENT_HIT_PROXY(HKzComboHitboxProxy, HHitProxy);

namespace
{
	constexpr float StepSeconds = 1.0f / 60.0f;

	// Short of the end, where an animation that loops starts over.
	constexpr float EndMargin = 1.0f / 120.0f;

	// Trail samples further apart are not swept between: the playhead jumped, which no game frame does.
	constexpr float MaxTrailGap = 0.1f;

	// Enough ways to a node for any combo a person draws, without walking a tangled graph forever.
	constexpr int32 MaxPaths = 16;
	constexpr int32 MaxDepth = 32;

	const FLinearColor HitColor(1.0f, 0.2f, 0.15f);
	const FLinearColor IdleColor(1.0f, 0.75f, 0.2f);
	const FLinearColor SelectedColor(0.3f, 0.8f, 1.0f);
	const FLinearColor TrailColor(0.55f, 0.12f, 0.08f);
	const FLinearColor TargetingColor(0.35f, 0.8f, 0.4f);

	bool IsComboWindow(const FKzAnimationNotifySpan& Span)
	{
		return Span.Notify && Span.Notify->IsA<UKzAnimNotifyState_ComboWindow>();
	}

	/** The Combo Window to press in: the one of that name, or for none the first unnamed one, else the first of all. */
	const FKzAnimationNotifySpan* FindComboWindow(const TArray<FKzAnimationNotifySpan>& Spans, FGameplayTag Name)
	{
		auto NameOf = [](const FKzAnimationNotifySpan& Span)
		{
			const UKzAnimNotifyState_ComboWindow* Window = Cast<UKzAnimNotifyState_ComboWindow>(Span.Notify);
			return Window ? Window->Window : FGameplayTag();
		};

		if (Name.IsValid())
		{
			return Spans.FindByPredicate([&NameOf, Name](const FKzAnimationNotifySpan& Span) { return IsComboWindow(Span) && NameOf(Span) == Name; });
		}
		const FKzAnimationNotifySpan* Unnamed = Spans.FindByPredicate([&NameOf](const FKzAnimationNotifySpan& Span) { return IsComboWindow(Span) && !NameOf(Span).IsValid(); });
		return Unnamed ? Unnamed : Spans.FindByPredicate(&IsComboWindow);
	}

	/** From when a press is kept for a Combo Window, as the game keeps it: InputBuffer before it opens, but not before the Hit Windows ahead of it close. */
	float GetBufferStart(const TArray<FKzAnimationNotifySpan>& Spans, const FKzAnimationNotifySpan& ComboWindow, float InputBuffer)
	{
		float Start = FMath::Max(ComboWindow.Start - InputBuffer, 0.0f);
		for (const FKzAnimationNotifySpan& Span : Spans)
		{
			if (Span.Notify && Span.Notify->IsA<UKzAnimNotifyState_HitWindow>() && Span.Start < ComboWindow.Start)
			{
				Start = FMath::Max(Start, Span.End);
			}
		}
		return Start;
	}

	/** The first attack an output leads to, through branches by their outputs in order. */
	UKzEdGraphNode* FindAttackFrom(UEdGraphPin& Output, EKzGraphFlow Flow, int32 Depth = 0)
	{
		if (Depth > MaxDepth)
		{
			return nullptr;
		}

		TArray<UEdGraphPin*> Targets;
		KzGraphEditor::GatherTargets(Output, Flow, Targets);
		for (UEdGraphPin* Input : Targets)
		{
			UKzEdGraphNode* Next = Cast<UKzEdGraphNode>(Input->GetOwningNode());
			if (Next && Next->Node.GetPtr<FKzComboNode_Attack>())
			{
				return Next;
			}
			if (Next && Next->Node.GetPtr<FKzComboNode_Branch>())
			{
				for (UEdGraphPin* BranchOutput : Next->Pins)
				{
					UKzEdGraphNode* Found = BranchOutput->Direction == EGPD_Output ? FindAttackFrom(*BranchOutput, Flow, Depth + 1) : nullptr;
					if (Found)
					{
						return Found;
					}
				}
			}
		}
		return nullptr;
	}

	/** The Entry of each of the combo's inputs, in their order. */
	TArray<UKzEdGraphNode*> FindEntries(const UEdGraph& Graph, const UKzComboGraph& Combo)
	{
		TArray<UKzEdGraphNode*> Entries;
		for (const FGameplayTag Input : Combo.Inputs)
		{
			for (UEdGraphNode* Each : Graph.Nodes)
			{
				UKzEdGraphNode* Node = Cast<UKzEdGraphNode>(Each);
				const FKzComboNode_Entry* Entry = Node ? Node->Node.GetPtr<FKzComboNode_Entry>() : nullptr;
				if (Entry && Entry->Input == Input)
				{
					Entries.AddUnique(Node);
					break;
				}
			}
		}
		return Entries;
	}

	// The attacks one after another, each with its windows as bands in their notify's color, and the playhead; dragging scrubs.
	class SKzComboTrack : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SKzComboTrack) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<FKzComboPreview>& InPreview)
		{
			Preview = InPreview;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(100.0, 28.0);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
			const FVector2f Size = AllottedGeometry.GetLocalSize();
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), White, ESlateDrawEffect::None, FLinearColor(0.015f, 0.015f, 0.015f));

			const TSharedPtr<FKzComboPreview> Pinned = Preview.Pin();
			const float Length = Pinned ? Pinned->GetLength() : 0.0f;
			if (Length <= 0.0f)
			{
				return LayerId;
			}

			// One lane per kind of window, so overlapping ones stay readable.
			TArray<const UClass*> Lanes;
			for (const FKzComboPreview::FStep& Step : Pinned->GetSteps())
			{
				for (const FKzAnimationNotifySpan& Span : Step.Notifies)
				{
					Lanes.AddUnique(Span.Notify ? Span.Notify->GetClass() : nullptr);
				}
			}
			const float LaneHeight = (Size.Y - 4.0f) / FMath::Max(Lanes.Num(), 1);
			auto ToX = [&Size, Length](float Time) { return Size.X * FMath::Clamp(Time / Length, 0.0f, 1.0f); };
			auto Band = [&](float From, float To, float Top, const FLinearColor& Color)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(FMath::Max(To - From, 2.0f), FMath::Max(LaneHeight - 1.0f, 1.0f)), FSlateLayoutTransform(FVector2f(From, Top))), White, ESlateDrawEffect::None, Color);
			};

			const TArray<FKzComboPreview::FStep>& Steps = Pinned->GetSteps();
			for (int32 Index = 0; Index < Steps.Num(); ++Index)
			{
				// Each attack shows until the next one takes over.
				const FKzComboPreview::FStep& Step = Steps[Index];
				for (const FKzAnimationNotifySpan& Span : Step.Notifies)
				{
					const float Start = ToX(FMath::Min(Step.Start + Span.Start, Step.End));
					const float End = ToX(FMath::Min(Step.Start + Span.End, Step.End));
					if (End <= Start)
					{
						continue;
					}
					const float Top = 2.0f + LaneHeight * Lanes.IndexOfByKey(Span.Notify ? Span.Notify->GetClass() : nullptr);
					const FLinearColor Color = Span.Notify ? FLinearColor(Span.Notify->NotifyColor) : FLinearColor::Gray;
					Band(Start, End, Top, Color.CopyWithNewOpacity(0.7f));

					// The input buffer leads into a Combo Window, fainter.
					const float BufferStart = IsComboWindow(Span) ? ToX(Step.Start + GetBufferStart(Step.Notifies, Span, Pinned->GetInputBuffer())) : Start;
					if (BufferStart < Start)
					{
						Band(BufferStart, Start, Top, Color.CopyWithNewOpacity(0.25f));
					}
				}

				if (Index > 0)
				{
					const float X = ToX(Step.Start);
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(X, 0.0f), FVector2f(X, Size.Y) }, ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, 0.35f), true, 1.0f);
				}
			}

			const float Playhead = ToX(Pinned->GetTime());
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(Playhead, 0.0f), FVector2f(Playhead, Size.Y) }, ESlateDrawEffect::None, FLinearColor::White, true, 2.0f);
			return LayerId + 3;
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
			{
				return FReply::Unhandled();
			}
			// Without this the editor stops ticking viewports while the mouse is held, and the pose only follows on release.
			ScrubTo(MyGeometry, MouseEvent);
			return FReply::Handled().CaptureMouse(SharedThis(this)).PreventThrottling();
		}

		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (!HasMouseCapture())
			{
				return FReply::Unhandled();
			}
			ScrubTo(MyGeometry, MouseEvent);
			return FReply::Handled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			return HasMouseCapture() ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled();
		}

	private:
		void ScrubTo(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
		{
			const TSharedPtr<FKzComboPreview> Pinned = Preview.Pin();
			const float Width = MyGeometry.GetLocalSize().X;
			if (Pinned && Width > 0.0f)
			{
				const float X = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
				Pinned->ScrubTo(Pinned->GetLength() * FMath::Clamp(X / Width, 0.0f, 1.0f));
			}
		}

		TWeakPtr<FKzComboPreview> Preview;
	};
}

void FKzComboPreview::Initialize(FAdvancedPreviewScene& InScene)
{
	Scene = &InScene;
	if (UKzComboGraph* Asset = Combo.Get())
	{
		NodeRenderer = FKzComboNodeRenderer::Get(*Asset);
		Stage.Update(InScene, *Asset);
	}
	BuildSteps();
}

void FKzComboPreview::OnSelectionChanged(const TSet<UObject*>& Selection)
{
	if (NodeRenderer)
	{
		NodeRenderer->SetSelection(Selection);
	}

	// Picking anything other than an attack keeps the last one shown, paused; picking one plays it from the start.
	UKzEdGraphNode* Node = Selection.Num() == 1 ? Cast<UKzEdGraphNode>(*Selection.CreateConstIterator()) : nullptr;
	if (!Node || !Node->Node.GetPtr<FKzComboNode_Attack>())
	{
		bPlaying = false;
		return;
	}

	if (Node != SelectedNode.Get())
	{
		SelectedNode = Node;
		PathIndex = 0;
		Time = 0.0f;
	}
	BuildSteps();
	bPlaying = true;
}

void FKzComboPreview::OnGraphChanged()
{
	const UKzComboGraph* Asset = Combo.Get();
	if (Scene && Asset)
	{
		Stage.Update(*Scene, *Asset);
	}
	BuildSteps();
}

void FKzComboPreview::Tick(float DeltaTime)
{
	// A Blueprint compiled since, as one playing in the editor does, may have replaced the stage.
	const UKzComboGraph* Asset = Combo.Get();
	if (Scene && Asset && Stage.Update(*Scene, *Asset))
	{
		BuildSteps();
	}

	if (Steps.IsEmpty())
	{
		return;
	}

	if (bPlaying)
	{
		Time += DeltaTime * Rate * (Animation ? Animation->GetPlayRate() : 1.0f);
		const float Length = GetLength();
		if (Time >= Length)
		{
			if (bLooping)
			{
				Time = Length > 0.0f ? FMath::Fmod(Time, Length) : 0.0f;
				Trails.Reset();
			}
			else
			{
				Time = FMath::Max(Length - EndMargin, 0.0f);
				bPlaying = false;
			}
		}
	}

	PoseAt(Time);
	RecordTrails();
}

void FKzComboPreview::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const FKzComboNode_Attack* Attack = GetShownAttack();
	const FStep* Step = GetCurrentStep();
	if (!Attack || !Step)
	{
		return;
	}

	// Where the attack looks for targets as it starts, pointing ahead as with no direction pressed. Not clickable: hit testing skips it.
	AActor* Character = Stage.GetCharacter();
	const UKzSensorComponent* Sensor = UKzSystemLibrary::FindComponentByGameplayTag<UKzSensorComponent>(Character, KzTags::Sensor::Targeting);
	const FTransform UserTransform = Character ? Character->GetActorTransform() : FTransform::Identity;
	FKzShapeInstance TargetingShape;
	FTransform TargetingTransform;
	if (!PDI->IsHitTesting() && UKzComboInstance::GetTargetingShape(*Attack, Sensor, UserTransform, UserTransform.GetUnitAxis(EAxis::X), TargetingShape, TargetingTransform))
	{
		TargetingShape.DrawWire(PDI, TargetingTransform, TargetingColor, 0.0f, SDPG_World);
	}

	const bool bHitting = Animation && FindHitWindow(Time - Step->Start) != INDEX_NONE;
	for (int32 Index = 0; Index < Attack->Hitboxes.Num(); ++Index)
	{
		const FKzComboHitbox& Hitbox = Attack->Hitboxes[Index];
		if (Trails.IsValidIndex(Index))
		{
			// Through the steps the game sweeps between two frames, so a pose that jumps shows no gap the game does not leave.
			const TArray<FTrailSample>& Trail = Trails[Index];
			for (int32 Sample = 0; Sample < Trail.Num(); ++Sample)
			{
				const FTrailSample& To = Trail[Sample];
				const FTrailSample& From = Sample > 0 && Trail[Sample - 1].Window == To.Window && To.Time - Trail[Sample - 1].Time <= MaxTrailGap ? Trail[Sample - 1] : To;
				const int32 SweepSteps = FKzComboHitbox::GetSweepSteps(From.Parent, To.Parent);
				for (int32 SweepStep = 1; SweepStep <= SweepSteps; ++SweepStep)
				{
					Hitbox.Shape.DrawWire(PDI, Hitbox.GetTransform(From.Parent, To.Parent, static_cast<float>(SweepStep) / SweepSteps), TrailColor, 0.0f, SDPG_Foreground);
				}
			}
		}

		if (const TOptional<FTransform> Transform = GetHitboxTransform(Hitbox))
		{
			// Thick lines while picking, so a thin outline is easy to click.
			const bool bSelected = Index == SelectedHitbox;
			const float Thickness = PDI->IsHitTesting() ? 6.0f : (bSelected || bHitting ? 1.5f : 0.75f);
			PDI->SetHitProxy(new HKzComboHitboxProxy(Index));
			Hitbox.Shape.DrawWire(PDI, *Transform, bSelected ? SelectedColor : (bHitting ? HitColor : IdleColor), Thickness, SDPG_Foreground);
			PDI->SetHitProxy(nullptr);
		}
	}
}

void FKzComboPreview::DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas)
{
	FText Message;
	if (!Stage.GetMesh())
	{
		Message = LOCTEXT("NoCharacter", "Pick a Preview Character or a Preview Mesh in the combo's details.");
	}
	else if (!SelectedNode.IsValid())
	{
		Message = LOCTEXT("NoAttack", "Select an attack to preview it.");
	}
	else if (!Animation)
	{
		Message = LOCTEXT("NoAnimation", "The attack's animation has nothing to play here.");
	}

	if (!Message.IsEmpty())
	{
		Canvas.DrawShadowedString(10.0f, 40.0f, *Message.ToString(), GEngine->GetSmallFont(), FLinearColor::White);
		return;
	}

	TArray<FText> Warnings;
	if (const FText Problem = Animation->GetProblem(); !Problem.IsEmpty())
	{
		Warnings.Add(Problem);
	}
	if (bComboMode && Paths.IsEmpty())
	{
		Warnings.Add(LOCTEXT("NoPath", "Nothing leads to this attack from an Entry, so it plays alone."));
	}
	for (const FStep& Step : Steps)
	{
		const UKzEdGraphNode* Node = Step.Node.Get();
		const FKzComboNode_Attack* Attack = Node ? Node->Node.GetPtr<FKzComboNode_Attack>() : nullptr;
		if (Step.bNoComboWindow && Attack && Attack->Animation)
		{
			Warnings.Add(FText::Format(LOCTEXT("NoComboWindow", "{0} has no Combo Window: in game the combo stops there."), Attack->Animation->GetDescription()));
		}
	}

	float Y = 40.0f;
	for (const FText& Warning : Warnings)
	{
		Canvas.DrawShadowedString(10.0f, Y, *Warning.ToString(), GEngine->GetSmallFont(), FLinearColor(1.0f, 0.8f, 0.2f));
		Y += 16.0f;
	}
}

FBox FKzComboPreview::GetBounds() const
{
	const USkeletalMeshComponent* Mesh = Stage.GetMesh();
	return Mesh ? Mesh->Bounds.GetBox() : FBox(ForceInit);
}

TSharedPtr<SWidget> FKzComboPreview::MakeBottomPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(6.0f, 2.0f))
		[
			MakeTransportBar()
		];
}

TSharedRef<SWidget> FKzComboPreview::MakeTransportBar()
{
	const TSharedRef<FKzComboPreview> Self = StaticCastSharedRef<FKzComboPreview>(AsShared());
	const TWeakPtr<FKzComboPreview> Weak = Self;

	auto Run = [Weak](TFunction<void(FKzComboPreview&)> Action)
	{
		return FOnClicked::CreateLambda([Weak, Action]()
		{
			if (const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin())
			{
				Action(*Pinned);
			}
			return FReply::Handled();
		});
	};
	auto While = [Weak](TFunction<bool(const FKzComboPreview&)> Test)
	{
		return TAttribute<bool>::CreateLambda([Weak, Test]()
		{
			const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
			return Pinned && Test(*Pinned);
		});
	};
	auto TransportButton = [](const FText& Tooltip, const TAttribute<const FSlateBrush*>& Image, const FOnClicked& OnClicked, const TAttribute<bool>& bEnabled)
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.IsFocusable(false)
			.ContentPadding(FMargin(6.0f, 4.0f))
			.ToolTipText(Tooltip)
			.IsEnabled(bEnabled)
			.OnClicked(OnClicked)
			[
				SNew(SImage)
				.Image(Image)
				.ColorAndOpacity(FSlateColor::UseForeground())
			];
	};
	const auto HasSteps = [](const FKzComboPreview& Preview) { return !Preview.Steps.IsEmpty(); };

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("RestartTooltip", "Back to the start."), FAppStyle::GetBrush("Animation.Backward_End"), Run([](FKzComboPreview& Preview) { Preview.Time = 0.0f; Preview.Trails.Reset(); }), While(HasSteps))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("PlayTooltip", "Play or pause."), TAttribute<const FSlateBrush*>::CreateLambda([Weak]()
			{
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				return FAppStyle::GetBrush(Pinned && Pinned->IsPlaying() ? "Animation.Pause" : "Animation.Forward");
			}), Run([](FKzComboPreview& Preview) { Preview.TogglePlay(); }), While(HasSteps))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("StepTooltip", "Move on by one frame while paused."), FAppStyle::GetBrush("Animation.Forward_Step"), Run([](FKzComboPreview& Preview)
			{
				Preview.ScrubTo(FMath::Min(Preview.GetTime() + StepSeconds, Preview.GetLength() - EndMargin));
			}), While([](const FKzComboPreview& Preview) { return !Preview.Steps.IsEmpty() && !Preview.IsPlaying(); }))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("EndTooltip", "Pause at the end."), FAppStyle::GetBrush("Animation.Forward_End"), Run([](FKzComboPreview& Preview)
			{
				Preview.ScrubTo(FMath::Max(Preview.GetLength() - EndMargin, 0.0f));
			}), While(HasSteps))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("LoopTooltip", "Play over and over; off, it stops at the end."), TAttribute<const FSlateBrush*>::CreateLambda([Weak]()
			{
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				return FAppStyle::GetBrush(Pinned && Pinned->bLooping ? "Animation.Loop.Enabled" : "Animation.Loop.Disabled");
			}), Run([](FKzComboPreview& Preview) { Preview.bLooping = !Preview.bLooping; }), true)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("ComboTooltip", "Play the attacks from an Entry up to the selected one, chained at the combo's Preview Chain Timing; off, the selected attack alone."))
			.IsChecked_Lambda([Weak]()
			{
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				return Pinned && Pinned->bComboMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([Weak](ECheckBoxState State)
			{
				if (const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin())
				{
					Pinned->bComboMode = State == ECheckBoxState::Checked;
					Pinned->PathIndex = 0;
					Pinned->Time = 0.0f;
					Pinned->BuildSteps();
				}
			})
			[
				SNew(STextBlock)
				.Margin(FMargin(6.0f, 2.0f))
				.Text(LOCTEXT("Combo", "Combo"))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SComboButton)
			.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
			.ToolTipText(LOCTEXT("PathTooltip", "Which way to the selected attack plays, by the outputs it takes."))
			.Visibility_Lambda([Weak]()
			{
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				return Pinned && Pinned->bComboMode && Pinned->Paths.Num() > 1 ? EVisibility::Visible : EVisibility::Collapsed;
			})
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text_Lambda([Weak]()
				{
					const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
					return Pinned && Pinned->Paths.IsValidIndex(Pinned->PathIndex) ? Pinned->Paths[Pinned->PathIndex].Label : FText::GetEmpty();
				})
			]
			.OnGetMenuContent_Lambda([Weak]()
			{
				FMenuBuilder Menu(true, nullptr);
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				for (int32 Index = 0; Pinned && Index < Pinned->Paths.Num(); ++Index)
				{
					Menu.AddMenuEntry(Pinned->Paths[Index].Label, FText::GetEmpty(), FSlateIcon(), FUIAction(
						FExecuteAction::CreateLambda([Weak, Index]()
						{
							if (const TSharedPtr<FKzComboPreview> Chosen = Weak.Pin())
							{
								Chosen->PathIndex = Index;
								Chosen->Time = 0.0f;
								Chosen->BuildSteps();
							}
						}),
						FCanExecuteAction(),
						FIsActionChecked::CreateLambda([Weak, Index]()
						{
							const TSharedPtr<FKzComboPreview> Chosen = Weak.Pin();
							return Chosen && Chosen->PathIndex == Index;
						})), NAME_None, EUserInterfaceActionType::RadioButton);
				}
				return Menu.MakeWidget();
			})
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		.Padding(6.0f, 0.0f)
		[
			SNew(SKzComboTrack, Self)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text_Lambda([Weak]()
			{
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				FNumberFormattingOptions Format;
				Format.SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);
				return FText::Format(LOCTEXT("Time", "{0} / {1} s"), FText::AsNumber(Pinned ? Pinned->GetTime() : 0.0f, &Format), FText::AsNumber(Pinned ? Pinned->GetLength() : 0.0f, &Format));
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SComboButton)
			.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
			.ToolTipText(LOCTEXT("RateTooltip", "How fast the preview plays; below 1 in slow motion."))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text_Lambda([Weak]()
				{
					const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
					return FText::Format(LOCTEXT("Rate", "{0}x"), FText::AsNumber(Pinned ? Pinned->Rate : 1.0f));
				})
			]
			.OnGetMenuContent_Lambda([Weak]()
			{
				FMenuBuilder Menu(true, nullptr);
				for (const float Option : { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f })
				{
					Menu.AddMenuEntry(FText::Format(LOCTEXT("Rate", "{0}x"), FText::AsNumber(Option)), FText::GetEmpty(), FSlateIcon(), FUIAction(
						FExecuteAction::CreateLambda([Weak, Option]()
						{
							if (const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin())
							{
								Pinned->Rate = Option;
							}
						}),
						FCanExecuteAction(),
						FIsActionChecked::CreateLambda([Weak, Option]()
						{
							const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
							return Pinned && Pinned->Rate == Option;
						})), NAME_None, EUserInterfaceActionType::RadioButton);
				}
				return Menu.MakeWidget();
			})
		];
}

void FKzComboPreview::AddReferencedObjects(FReferenceCollector& Collector)
{
	Stage.AddReferencedObjects(Collector);
}

float FKzComboPreview::GetLength() const
{
	return Steps.IsEmpty() ? 0.0f : Steps.Last().End;
}

float FKzComboPreview::GetInputBuffer() const
{
	const UKzComboGraph* Asset = Combo.Get();
	return Asset ? Asset->InputBuffer : 0.0f;
}

void FKzComboPreview::TogglePlay()
{
	// Playing from the end starts over.
	if (!bPlaying && Time >= GetLength() - EndMargin * 2.0f)
	{
		Time = 0.0f;
		Trails.Reset();
	}
	bPlaying = !bPlaying;
}

void FKzComboPreview::ScrubTo(float InTime)
{
	bPlaying = false;
	Time = FMath::Clamp(InTime, 0.0f, FMath::Max(GetLength() - EndMargin, 0.0f));
	PoseAt(Time);
}

void FKzComboPreview::BuildSteps()
{
	Steps.Reset();
	CurrentStep = INDEX_NONE;
	Animation.Reset();
	PreviousAnimation.Reset();
	Trails.Reset();
	Paths.Reset();

	// Until an attack is picked, the one the combo starts with.
	if (!SelectedNode.IsValid())
	{
		SelectedNode = FindFirstAttack();
	}

	const UKzComboGraph* Asset = Combo.Get();
	UKzEdGraphNode* Selected = SelectedNode.Get();
	if (!Asset || !Stage.GetMesh() || !Selected)
	{
		return;
	}

	TArray<TWeakObjectPtr<UKzEdGraphNode>> Nodes;
	TArray<FGameplayTag> Windows;
	if (bComboMode)
	{
		FindPaths();
		PathIndex = Paths.IsValidIndex(PathIndex) ? PathIndex : 0;
		if (Paths.IsValidIndex(PathIndex))
		{
			Nodes = Paths[PathIndex].Nodes;
			Windows = Paths[PathIndex].Windows;
		}
	}
	if (Nodes.IsEmpty())
	{
		Nodes.Add(Selected);
	}

	// Each attack gives way to the next where the preview's press lands in its Combo Window.
	const FKzAnimationContext Context = MakeContext();
	float Start = 0.0f;
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		UKzEdGraphNode* Node = Nodes[Index].Get();
		const FKzComboNode_Attack* Attack = Node ? Node->Node.GetPtr<FKzComboNode_Attack>() : nullptr;
		if (!Attack || !Attack->Animation)
		{
			break;
		}

		FStep& Step = Steps.AddDefaulted_GetRef();
		Step.Node = Node;
		Step.Start = Start;
		Step.Length = Attack->Animation->GetTimeline(Context, Step.Notifies);
		Step.End = Start + Step.Length;
		if (Index < Nodes.Num() - 1)
		{
			const FKzAnimationNotifySpan* Window = FindComboWindow(Step.Notifies, Windows.IsValidIndex(Index + 1) ? Windows[Index + 1] : FGameplayTag());
			Step.bNoComboWindow = !Window;
			if (Window)
			{
				Step.End = Start + FMath::Lerp(Window->Start, Window->End, Asset->PreviewChainTiming);
			}
		}
		Start = Step.End;
	}

	Time = FMath::Clamp(Time, 0.0f, FMath::Max(GetLength() - EndMargin, 0.0f));
	PoseAt(Time);
}

void FKzComboPreview::FindPaths()
{
	Paths.Reset();
	UKzEdGraphNode* Target = SelectedNode.Get();
	const UKzComboGraph* Asset = Combo.Get();
	const UEdGraph* Graph = Target ? Target->GetGraph() : nullptr;
	if (!Graph || !Asset)
	{
		return;
	}

	TArray<UKzEdGraphNode*> Walked;
	TArray<FGameplayTag> Windows;
	TArray<FText> Outputs;
	TArray<UKzEdGraphNode*> Branches;
	TFunction<void(UKzEdGraphNode&)> Walk;
	TFunction<void(UEdGraphPin&, FGameplayTag)> Follow;
	Follow = [&](UEdGraphPin& Output, FGameplayTag Window)
	{
		TArray<UEdGraphPin*> Targets;
		KzGraphEditor::GatherTargets(Output, Asset->GetFlow(), Targets);
		for (UEdGraphPin* Input : Targets)
		{
			UKzEdGraphNode* Next = Cast<UKzEdGraphNode>(Input->GetOwningNode());
			if (!Next || Paths.Num() >= MaxPaths)
			{
				continue;
			}

			// A branch is gone through by each of its outputs, and a Switch on Window's names the window the press comes in.
			if (Next->Node.GetPtr<FKzComboNode_Branch>())
			{
				if (Branches.Contains(Next))
				{
					continue;
				}

				Branches.Add(Next);
				const bool bByWindow = Next->Node.GetPtr<FKzComboNode_SwitchOnWindow>() != nullptr;
				for (UEdGraphPin* BranchOutput : Next->Pins)
				{
					if (BranchOutput->Direction == EGPD_Output)
					{
						Outputs.Add(BranchOutput->PinFriendlyName);
						Follow(*BranchOutput, bByWindow ? FGameplayTag::RequestGameplayTag(BranchOutput->PinName, false) : Window);
						Outputs.Pop();
					}
				}
				Branches.Pop();
				continue;
			}

			if (Walked.Contains(Next) || !Next->Node.GetPtr<FKzComboNode_Attack>())
			{
				continue;
			}

			Walked.Add(Next);
			Windows.Add(Window);
			if (Next == Target)
			{
				FPath& Path = Paths.AddDefaulted_GetRef();
				Path.Nodes.Append(Walked);
				Path.Windows = Windows;
				Path.Label = FText::Join(FText::FromString(TEXT(" > ")), Outputs.FilterByPredicate([](const FText& Part) { return !Part.IsEmpty(); }));
			}
			else if (Walked.Num() < MaxDepth)
			{
				Walk(*Next);
			}
			Walked.Pop();
			Windows.Pop();
		}
	};
	Walk = [&](UKzEdGraphNode& From)
	{
		for (UEdGraphPin* Output : From.Pins)
		{
			if (Output->Direction == EGPD_Output && Paths.Num() < MaxPaths)
			{
				Outputs.Add(Output->PinFriendlyName);
				Follow(*Output, FGameplayTag());
				Outputs.Pop();
			}
		}
	};
	for (UKzEdGraphNode* Entry : FindEntries(*Graph, *Asset))
	{
		Walk(*Entry);
	}

	Paths.StableSort([](const FPath& A, const FPath& B) { return A.Nodes.Num() < B.Nodes.Num(); });
}

UKzEdGraphNode* FKzComboPreview::FindFirstAttack() const
{
	const UKzComboGraph* Asset = Combo.Get();
	if (!Asset || !Asset->EdGraph)
	{
		return nullptr;
	}

	for (UKzEdGraphNode* Entry : FindEntries(*Asset->EdGraph, *Asset))
	{
		for (UEdGraphPin* Output : Entry->Pins)
		{
			UKzEdGraphNode* Found = Output->Direction == EGPD_Output ? FindAttackFrom(*Output, Asset->GetFlow()) : nullptr;
			if (Found)
			{
				return Found;
			}
		}
	}
	return nullptr;
}

void FKzComboPreview::PoseAt(float InTime)
{
	int32 Index = Steps.IndexOfByPredicate([InTime](const FStep& Step) { return InTime < Step.End; });
	if (Index == INDEX_NONE)
	{
		Index = Steps.Num() - 1;
	}
	if (!Steps.IsValidIndex(Index))
	{
		return;
	}

	if (Index != CurrentStep || !Animation)
	{
		CurrentStep = Index;
		Trails.Reset();
		if (GetShownNode() != SelectedHitboxNode.Get())
		{
			SelectedHitbox = INDEX_NONE;
		}

		const FKzComboNode_Attack* Attack = GetShownAttack();
		USkeletalMeshComponent* Mesh = Stage.GetMesh();

		// Made first: each preview takes the mesh over, and the shown attack's has to be the last.
		const UKzEdGraphNode* PreviousNode = Index > 0 ? Steps[Index - 1].Node.Get() : nullptr;
		const FKzComboNode_Attack* PreviousAttack = PreviousNode ? PreviousNode->Node.GetPtr<FKzComboNode_Attack>() : nullptr;
		PreviousAnimation = PreviousAttack && PreviousAttack->Animation && Mesh ? KzAnimationPreview::Make(*PreviousAttack->Animation, MakeContext(), *Mesh) : nullptr;

		Animation = Attack && Attack->Animation && Mesh ? KzAnimationPreview::Make(*Attack->Animation, MakeContext(), *Mesh) : nullptr;
		if (Animation)
		{
			Animation->SetPlaying(false);
		}
		if (NodeRenderer)
		{
			NodeRenderer->SetPlayingNode(bComboMode ? GetShownNode() : nullptr);
		}
	}

	if (Animation)
	{
		const float Elapsed = InTime - Steps[Index].Start;
		Animation->SetTime(Elapsed);

		// In game the attack before goes on underneath while this one blends in, by this one's Blend In.
		const float FromWeight = PreviousAnimation ? 1.0f - Animation->GetBlendInAlpha(Elapsed) : 0.0f;
		Animation->SetBlendFrom(PreviousAnimation.Get(), Index > 0 ? InTime - Steps[Index - 1].Start : 0.0f, FromWeight);
	}
}

FKzAnimationContext FKzComboPreview::MakeContext() const
{
	const UKzComboGraph* Asset = Combo.Get();
	FKzAnimationContext Context;
	Context.Mesh = Stage.GetMesh();
	Context.User = Stage.GetCharacter();
	Context.Item = Asset ? Asset->PreviewItem.LoadSynchronous() : nullptr;
	return Context;
}

const FKzComboPreview::FStep* FKzComboPreview::GetCurrentStep() const
{
	return Steps.IsValidIndex(CurrentStep) ? &Steps[CurrentStep] : nullptr;
}

UKzEdGraphNode* FKzComboPreview::GetShownNode() const
{
	const FStep* Step = GetCurrentStep();
	return Step ? Step->Node.Get() : nullptr;
}

const FKzComboNode_Attack* FKzComboPreview::GetShownAttack() const
{
	const UKzEdGraphNode* Node = GetShownNode();
	return Node ? Node->Node.GetPtr<FKzComboNode_Attack>() : nullptr;
}

const USceneComponent* FKzComboPreview::GetHitboxParent(const FKzComboHitbox& Hitbox) const
{
	return Hitbox.FindParent(Stage.GetMesh(), Stage.GetWeaponRoot());
}

TOptional<FTransform> FKzComboPreview::GetHitboxTransform(const FKzComboHitbox& Hitbox) const
{
	const USceneComponent* Parent = GetHitboxParent(Hitbox);
	return Parent ? Hitbox.GetTransform(*Parent) : TOptional<FTransform>();
}

bool FKzComboPreview::HandleClick(HHitProxy* HitProxy)
{
	if (const HKzComboHitboxProxy* Proxy = HitProxyCast<HKzComboHitboxProxy>(HitProxy))
	{
		SelectedHitbox = Proxy->Index;
		SelectedHitboxNode = GetShownNode();
		return true;
	}
	SelectedHitbox = INDEX_NONE;
	return false;
}

TOptional<FTransform> FKzComboPreview::GetGizmoTransform() const
{
	const FKzComboNode_Attack* Attack = GetShownAttack();
	return Attack && Attack->Hitboxes.IsValidIndex(SelectedHitbox) ? GetHitboxTransform(Attack->Hitboxes[SelectedHitbox]) : TOptional<FTransform>();
}

void FKzComboPreview::MoveGizmo(const FVector& Drag, const FRotator& Rotation)
{
	UKzEdGraphNode* Node = GetShownNode();
	FKzComboNode_Attack* Attack = Node ? Node->Node.GetMutablePtr<FKzComboNode_Attack>() : nullptr;
	if (!Attack || !Attack->Hitboxes.IsValidIndex(SelectedHitbox))
	{
		return;
	}

	FKzComboHitbox& Hitbox = Attack->Hitboxes[SelectedHitbox];
	const USceneComponent* Parent = GetHitboxParent(Hitbox);
	if (!Parent)
	{
		return;
	}

	const FTransform ParentTransform = Parent->GetSocketTransform(Hitbox.Socket);
	FTransform World = Hitbox.Offset * ParentTransform;
	World.AddToTranslation(Drag);
	World.SetRotation(Rotation.Quaternion() * World.GetRotation());
	Hitbox.Offset = World.GetRelativeTransform(ParentTransform);
}

void FKzComboPreview::BeginGizmoDrag()
{
	GizmoTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("MoveHitbox", "Move Hitbox"));
	if (UKzEdGraphNode* Node = GetShownNode())
	{
		Node->Modify();
	}
}

void FKzComboPreview::EndGizmoDrag()
{
	if (UKzEdGraphNode* Node = GetShownNode())
	{
		Node->PostEditChange();
	}
	GizmoTransaction.Reset();
}

int32 FKzComboPreview::FindHitWindow(float LocalTime) const
{
	const FStep* Step = GetCurrentStep();
	return Step ? Step->Notifies.IndexOfByPredicate([LocalTime](const FKzAnimationNotifySpan& Span)
	{
		return Span.Notify && Span.Notify->IsA<UKzAnimNotifyState_HitWindow>() && LocalTime >= Span.Start && LocalTime <= Span.End;
	}) : INDEX_NONE;
}

void FKzComboPreview::RecordTrails()
{
	const FKzComboNode_Attack* Attack = GetShownAttack();
	const FStep* Step = GetCurrentStep();
	if (!Attack || !Step || !Animation)
	{
		return;
	}

	Trails.SetNum(Attack->Hitboxes.Num());
	const float LocalTime = Time - Step->Start;
	const int32 Window = FindHitWindow(LocalTime);
	if (Window == INDEX_NONE)
	{
		return;
	}

	for (int32 Index = 0; Index < Attack->Hitboxes.Num(); ++Index)
	{
		const FKzComboHitbox& Hitbox = Attack->Hitboxes[Index];
		const USceneComponent* Parent = GetHitboxParent(Hitbox);
		if (!Parent)
		{
			continue;
		}

		// The parent rather than the hitbox, so the trail follows the hitbox as the gizmo moves it.
		const FTransform ParentTransform = Parent->GetSocketTransform(Hitbox.Socket);

		// One sample per frame of the animation, the latest kept, so playing the window again does not pile them up.
		TArray<FTrailSample>& Trail = Trails[Index];
		FTrailSample* Same = Trail.FindByPredicate([LocalTime](const FTrailSample& Sample) { return FMath::Abs(Sample.Time - LocalTime) < StepSeconds * 0.5f; });
		if (Same)
		{
			Same->Parent = ParentTransform;
		}
		else
		{
			Trail.Insert({ LocalTime, Window, ParentTransform }, Algo::LowerBoundBy(Trail, LocalTime, &FTrailSample::Time));
		}
	}
}

#undef LOCTEXT_NAMESPACE