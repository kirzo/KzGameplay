// Copyright 2026 kirzo

#include "Combat/KzComboNodeBodies.h"
#include "Animation/KzAnimationNotifyState.h"
#include "Animation/KzAnimationSource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodeRenderer.h"
#include "Combat/KzComboNodes.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Items/KzItemDefinition.h"
#include "KzEdGraphNode.h"
#include "KzGraphEditorUtils.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/GCObject.h"

#define LOCTEXT_NAMESPACE "KzComboNodeBodies"

namespace
{
	constexpr int32 BodyWidth = FKzComboNodeRenderer::Width;
	constexpr int32 SettleFrames = 30;
	const FLinearColor ChipTextColor(FColor(0xDD, 0xE1, 0xE6));
	const FLinearColor WarningColor(0.95f, 0.45f, 0.2f);
	const FLinearColor PlayingColor(FColor(0x34, 0xD1, 0xE0));

	const FSlateBrush* GetChipBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(FColor(0x30, 0x35, 0x3C)), 4.0f);
		return &Brush;
	}

	/** The combo's preview item answers By Query animations, as in the viewport. */
	FKzAnimationContext MakeContext(const UKzEdGraphNode& Node)
	{
		FKzAnimationContext Context;
		const UKzComboGraph* Combo = Cast<UKzComboGraph>(KzGraphEditor::GetGraphAsset(Node.GetGraph()));
		Context.Item = Combo ? Combo->PreviewItem.LoadSynchronous() : nullptr;
		return Context;
	}

	/** The attack on the combo's preview character: posed where it shows best, and playing while hovered or selected. */
	class SKzAttackPreview : public SLeafWidget, public FGCObject
	{
	public:
		SLATE_BEGIN_ARGS(SKzAttackPreview) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const UKzEdGraphNode& InNode, const TSharedRef<FKzComboNodeRenderer>& InRenderer)
		{
			Node = &InNode;
			Renderer = InRenderer;
			Renderer->Invalidate(InNode);

			Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
			Target->ClearColor = FLinearColor(0.015f, 0.015f, 0.015f);
			Target->InitCustomFormat(FKzComboNodeRenderer::Width, FKzComboNodeRenderer::Height, PF_B8G8R8A8, false);
			Target->UpdateResourceImmediate(true);

			Brush.SetResourceObject(Target);
			Brush.ImageSize = FVector2D(FKzComboNodeRenderer::Width, FKzComboNodeRenderer::Height);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(FKzComboNodeRenderer::Width, FKzComboNodeRenderer::Height);
		}

		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
		{
			const UKzEdGraphNode* EdNode = Node.Get();
			if (!EdNode || !Target)
			{
				return;
			}

			// One capture comes out dark and grainy until exposure and the temporal effects settle, so the rest pose is shot for a while.
			const bool bLive = IsHovered() || Renderer->IsSelected(*EdNode);
			if (bLive)
			{
				Time = bWasLive ? Time + InDeltaTime : 0.0f;
				Renderer->Request(*EdNode, Time, *Target);
				PoseFrames = 0;
			}
			else if (PoseFrames < SettleFrames)
			{
				Renderer->Request(*EdNode, TOptional<float>(), *Target);
				PoseFrames++;
			}
			bWasLive = bLive;
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &Brush);

			// The attack the viewport is playing in a whole combo, framed so the graph shows how far it went.
			const UKzEdGraphNode* EdNode = Node.Get();
			if (!EdNode || !Renderer->IsPlaying(*EdNode))
			{
				return LayerId;
			}

			const FVector2f Size = AllottedGeometry.GetLocalSize();
			const TArray<FVector2f> Frame{ FVector2f(1.0f, 1.0f), FVector2f(Size.X - 1.0f, 1.0f), FVector2f(Size.X - 1.0f, Size.Y - 1.0f), FVector2f(1.0f, Size.Y - 1.0f), FVector2f(1.0f, 1.0f) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Frame, ESlateDrawEffect::None, PlayingColor, true, 2.0f);
			return LayerId + 1;
		}

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Collector.AddReferencedObject(Target);
		}

		virtual FString GetReferencerName() const override
		{
			return TEXT("SKzAttackPreview");
		}

	private:
		TWeakObjectPtr<const UKzEdGraphNode> Node;
		TSharedPtr<FKzComboNodeRenderer> Renderer;
		TObjectPtr<UTextureRenderTarget2D> Target;
		FSlateBrush Brush;
		float Time = 0.0f;
		bool bWasLive = false;
		int32 PoseFrames = 0;
	};

	/** The attack's windows along its length, one lane per kind, as the viewport's track shows them. */
	class SKzAttackWindows : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SKzAttackWindows) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TArray<FKzAnimationNotifySpan>& Spans, float Length)
		{
			// Copied out, so an edited animation cannot leave the node pointing at notifies that are gone.
			TArray<const UClass*> Kinds;
			for (const FKzAnimationNotifySpan& Span : Spans)
			{
				const int32 Lane = Kinds.AddUnique(Span.Notify ? Span.Notify->GetClass() : nullptr);
				Bands.Add({ FMath::Clamp(Span.Start / Length, 0.0f, 1.0f), FMath::Clamp(Span.End / Length, 0.0f, 1.0f), Lane, Span.Notify ? FLinearColor(Span.Notify->NotifyColor) : FLinearColor::Gray });
			}
			Lanes = FMath::Max(Kinds.Num(), 1);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(BodyWidth, 2.0 + 4.0 * Lanes);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
			const FVector2f Size = AllottedGeometry.GetLocalSize();
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), White, ESlateDrawEffect::None, FLinearColor(0.015f, 0.015f, 0.015f));

			const float LaneHeight = (Size.Y - 2.0f) / Lanes;
			for (const FBand& Band : Bands)
			{
				const float Start = Size.X * Band.Start;
				const FVector2f BandSize(FMath::Max(Size.X * Band.End - Start, 2.0f), FMath::Max(LaneHeight - 1.0f, 1.0f));
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(BandSize, FSlateLayoutTransform(FVector2f(Start, 1.0f + LaneHeight * Band.Lane))), White, ESlateDrawEffect::None, Band.Color);
			}
			return LayerId + 1;
		}

	private:
		struct FBand
		{
			float Start = 0.0f;
			float End = 0.0f;
			int32 Lane = 0;
			FLinearColor Color;
		};

		TArray<FBand> Bands;
		int32 Lanes = 1;
	};

	TSharedRef<SWidget> MakeChip(const FText& Text, const FLinearColor& Color = ChipTextColor, const FText& ToolTip = FText::GetEmpty())
	{
		return SNew(SBorder)
			.BorderImage(GetChipBrush())
			.Padding(FMargin(5.0f, 1.0f))
			.ToolTipText(ToolTip)
			[
				SNew(STextBlock)
				.Text(Text)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(Color)
			];
	}


	/** The animation's thumbnail, its windows, and chips for what the attack does. */
	TSharedPtr<SWidget> MakeAttackBody(const UKzEdGraphNode& Node)
	{
		const FKzComboNode_Attack* Attack = Node.Node.GetPtr<FKzComboNode_Attack>();
		if (!Attack)
		{
			return nullptr;
		}

		const FKzAnimationContext Context = MakeContext(Node);
		TArray<FKzAnimationNotifySpan> Spans;
		const float Length = Attack->Animation ? Attack->Animation->GetTimeline(Context, Spans) : 0.0f;

		const TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
		if (UKzComboGraph* Combo = Attack->Animation ? Cast<UKzComboGraph>(KzGraphEditor::GetGraphAsset(Node.GetGraph())) : nullptr)
		{
			Body->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SKzAttackPreview, Node, FKzComboNodeRenderer::Get(*Combo))
			];
		}

		if (Length > 0.0f)
		{
			Body->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SKzAttackWindows, Spans, Length)
			];
		}

		const TSharedRef<SWrapBox> Chips = SNew(SWrapBox)
			.PreferredSize(BodyWidth)
			.InnerSlotPadding(FVector2D(3.0, 3.0));
		auto AddChip = [&Chips](const FText& Text, const FLinearColor& Color = ChipTextColor, const FText& ToolTip = FText::GetEmpty())
		{
			Chips->AddSlot()[MakeChip(Text, Color, ToolTip)];
		};

		AddChip(FText::Format(LOCTEXT("Damage", "x{0} damage"), FText::AsNumber(Attack->DamageMultiplier)));
		if (Attack->Hitboxes.IsEmpty())
		{
			AddChip(LOCTEXT("NoHitboxes", "No hitboxes"), WarningColor, LOCTEXT("NoHitboxesTip", "It hits nothing: add hitboxes in its details."));
		}
		else
		{
			AddChip(FText::Format(LOCTEXT("Hitboxes", "{0} {0}|plural(one=hitbox,other=hitboxes)"), Attack->Hitboxes.Num()));
		}
		for (const FGameplayTag Tag : Attack->AttackTags)
		{
			AddChip(FText::FromName(Tag.GetTagLeafName()));
		}
		if (Attack->HitStop > 0.0f)
		{
			AddChip(FText::Format(LOCTEXT("HitStop", "Hit stop {0} s"), FText::AsNumber(Attack->HitStop)));
		}
		if (!Attack->bFaceTarget)
		{
			AddChip(LOCTEXT("NoFacing", "No turning"), ChipTextColor, LOCTEXT("NoFacingTip", "It does not turn to its target."));
		}

		Body->AddSlot()
		.AutoHeight()
		[
			Chips
		];

		return SNew(SBox)
			.Padding(FMargin(6.0f, 4.0f, 6.0f, 2.0f))
			[
				Body
			];
	}

	void OpenAnimation(const UKzEdGraphNode& Node)
	{
		const FKzComboNode_Attack* Attack = Node.Node.GetPtr<FKzComboNode_Attack>();
		UObject* Asset = Attack && Attack->Animation ? Attack->Animation->GetAsset(MakeContext(Node)) : nullptr;
		if (Asset && GEditor)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
		}
	}
}

void KzComboNodeBodies::Register()
{
	KzGraphEditor::RegisterNodeBody(FKzComboNode_Attack::StaticStruct(), FKzOnMakeGraphNodeBody::CreateStatic(&MakeAttackBody));
	KzGraphEditor::RegisterNodeDoubleClick(FKzComboNode_Attack::StaticStruct(), FKzOnGraphNodeDoubleClicked::CreateStatic(&OpenAnimation));
}

void KzComboNodeBodies::Unregister()
{
	KzGraphEditor::UnregisterNodeBody(FKzComboNode_Attack::StaticStruct());
	KzGraphEditor::UnregisterNodeDoubleClick(FKzComboNode_Attack::StaticStruct());
}

#undef LOCTEXT_NAMESPACE