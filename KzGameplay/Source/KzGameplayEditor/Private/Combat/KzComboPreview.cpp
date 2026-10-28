// Copyright 2026 kirzo

#include "Combat/KzComboPreview.h"
#include "AdvancedPreviewScene.h"
#include "Animation/KzAnimationNotifyState.h"
#include "Animation/KzAnimationPreview.h"
#include "CanvasTypes.h"
#include "Combat/KzCombatNotifies.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Equipment/KzEquipmentComponent.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Character.h"
#include "HitProxies.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Items/KzItemDefinition.h"
#include "KzEdGraphNode.h"
#include "SceneManagement.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
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

	const FLinearColor HitColor(1.0f, 0.2f, 0.15f);
	const FLinearColor IdleColor(1.0f, 0.75f, 0.2f);
	const FLinearColor SelectedColor(0.3f, 0.8f, 1.0f);
	const FLinearColor TrailColor(0.55f, 0.12f, 0.08f);

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

	// The attack's length with its windows as bands in their notify's color, and the playhead; dragging scrubs.
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
			for (const FKzAnimationNotifySpan& Span : Pinned->GetNotifies())
			{
				Lanes.AddUnique(Span.Notify ? Span.Notify->GetClass() : nullptr);
			}
			const float LaneHeight = (Size.Y - 4.0f) / FMath::Max(Lanes.Num(), 1);

			for (const FKzAnimationNotifySpan& Span : Pinned->GetNotifies())
			{
				const float Start = Size.X * FMath::Clamp(Span.Start / Length, 0.0f, 1.0f);
				const float End = Size.X * FMath::Clamp(Span.End / Length, 0.0f, 1.0f);
				const float Top = 2.0f + LaneHeight * Lanes.IndexOfByKey(Span.Notify ? Span.Notify->GetClass() : nullptr);
				const FLinearColor Color = Span.Notify ? FLinearColor(Span.Notify->NotifyColor) : FLinearColor::Gray;
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(FMath::Max(End - Start, 2.0f), FMath::Max(LaneHeight - 1.0f, 1.0f)), FSlateLayoutTransform(FVector2f(Start, Top))), White, ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.7f));
			}

			const float Playhead = Size.X * FMath::Clamp(Pinned->GetTime() / Length, 0.0f, 1.0f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(Playhead, 0.0f), FVector2f(Playhead, Size.Y) }, ESlateDrawEffect::None, FLinearColor::White, true, 2.0f);
			return LayerId + 2;
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
	ShowCharacter();
	ShowItem();
	ShowAnimation();
}

void FKzComboPreview::OnSelectionChanged(const TSet<UObject*>& Selection)
{
	// Picking anything other than an attack keeps the last one shown.
	UKzEdGraphNode* Node = Selection.Num() == 1 ? Cast<UKzEdGraphNode>(*Selection.CreateConstIterator()) : nullptr;
	if (Node && Node->Node.GetPtr<FKzComboNode_Attack>())
	{
		if (Node != SelectedNode.Get())
		{
			SelectedHitbox = INDEX_NONE;
		}
		SelectedNode = Node;
		ShowAnimation();
	}
}

void FKzComboPreview::OnGraphChanged()
{
	const UKzComboGraph* Asset = Combo.Get();
	const bool bNewCharacter = Asset && (Asset->PreviewCharacter != ShownCharacter || Asset->PreviewMesh != ShownMesh);
	if (bNewCharacter)
	{
		ShowCharacter();
	}
	if (Asset && (bNewCharacter || Asset->PreviewItem != ShownItem))
	{
		ShowItem();
	}
	ShowAnimation();
}

void FKzComboPreview::Tick(float DeltaTime)
{
	if (!Animation)
	{
		return;
	}

	Animation->Tick(DeltaTime);

	// Without the loop, an attack that went round stops at its end.
	const float Time = Animation->GetTime();
	if (!bLooping && Animation->IsPlaying() && Time < LastTime)
	{
		Animation->SetPlaying(false);
		Animation->SetTime(FMath::Max(Animation->GetLength() - EndMargin, 0.0f));
	}
	LastTime = Animation->GetTime();

	RecordTrails();
}

void FKzComboPreview::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const FKzComboNode_Attack* Attack = GetSelectedAttack();
	if (!Attack)
	{
		return;
	}

	const bool bHitting = Animation && IsInHitWindow(Animation->GetTime());
	for (int32 Index = 0; Index < Attack->Hitboxes.Num(); ++Index)
	{
		const FKzComboHitbox& Hitbox = Attack->Hitboxes[Index];
		if (Trails.IsValidIndex(Index))
		{
			for (const FTrailSample& Sample : Trails[Index])
			{
				Hitbox.Shape.DrawWire(PDI, Sample.Transform, TrailColor, 0.0f, SDPG_Foreground);
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
	if (!Mesh)
	{
		Message = LOCTEXT("NoCharacter", "Pick a Preview Character or a Preview Mesh in the combo's details.");
	}
	else if (!GetSelectedAttack())
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
	}
	else if (const FText Problem = Animation->GetProblem(); !Problem.IsEmpty())
	{
		Canvas.DrawShadowedString(10.0f, 40.0f, *Problem.ToString(), GEngine->GetSmallFont(), FLinearColor(1.0f, 0.8f, 0.2f));
	}
}

FBox FKzComboPreview::GetBounds() const
{
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
	const auto HasAnimation = [](const FKzComboPreview& Preview) { return Preview.Animation.IsValid(); };

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("RestartTooltip", "Back to the attack's start."), FAppStyle::GetBrush("Animation.Backward_End"), Run([](FKzComboPreview& Preview) { Preview.Animation->SetTime(0.0f); }), While(HasAnimation))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("PlayTooltip", "Play or pause the attack."), TAttribute<const FSlateBrush*>::CreateLambda([Weak]()
			{
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				return FAppStyle::GetBrush(Pinned && Pinned->IsPlaying() ? "Animation.Pause" : "Animation.Forward");
			}), Run([](FKzComboPreview& Preview) { Preview.TogglePlay(); }), While(HasAnimation))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("StepTooltip", "Move the paused attack on by one frame."), FAppStyle::GetBrush("Animation.Forward_Step"), Run([](FKzComboPreview& Preview)
			{
				Preview.ScrubTo(FMath::Min(Preview.GetTime() + StepSeconds, Preview.GetLength() - EndMargin));
			}), While([](const FKzComboPreview& Preview) { return Preview.Animation.IsValid() && !Preview.IsPlaying(); }))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("EndTooltip", "Pause the attack at its end."), FAppStyle::GetBrush("Animation.Forward_End"), Run([](FKzComboPreview& Preview)
			{
				Preview.ScrubTo(FMath::Max(Preview.GetLength() - EndMargin, 0.0f));
			}), While(HasAnimation))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TransportButton(LOCTEXT("LoopTooltip", "Play the attack over and over; off, it stops at its end."), TAttribute<const FSlateBrush*>::CreateLambda([Weak]()
			{
				const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
				return FAppStyle::GetBrush(Pinned && Pinned->bLooping ? "Animation.Loop.Enabled" : "Animation.Loop.Disabled");
			}), Run([](FKzComboPreview& Preview) { Preview.bLooping = !Preview.bLooping; }), true)
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
								if (Pinned->Animation)
								{
									Pinned->Animation->SetRate(Option);
								}
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
	Collector.AddReferencedObject(Character);
	Collector.AddReferencedObject(Mesh);
	Collector.AddReferencedObject(Weapon);
	Collector.AddReferencedObject(WeaponMesh);
}

float FKzComboPreview::GetLength() const
{
	return Animation ? Animation->GetLength() : 0.0f;
}

float FKzComboPreview::GetTime() const
{
	return Animation ? Animation->GetTime() : 0.0f;
}

bool FKzComboPreview::IsPlaying() const
{
	return Animation && Animation->IsPlaying();
}

void FKzComboPreview::TogglePlay()
{
	if (Animation)
	{
		Animation->SetPlaying(!Animation->IsPlaying());
	}
}

void FKzComboPreview::ScrubTo(float Time)
{
	if (Animation)
	{
		Animation->SetPlaying(false);
		Animation->SetTime(Time);
	}
}

void FKzComboPreview::ShowCharacter()
{
	const UKzComboGraph* Asset = Combo.Get();
	if (!Scene || !Asset)
	{
		return;
	}

	if (Character)
	{
		Character->Destroy();
	}
	else if (Mesh)
	{
		Scene->RemoveComponent(Mesh);
	}
	Character = nullptr;
	Mesh = nullptr;
	Animation.Reset();

	ShownCharacter = Asset->PreviewCharacter;
	ShownMesh = Asset->PreviewMesh;

	if (UClass* Class = ShownCharacter.LoadSynchronous())
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags = RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character = Scene->GetWorld()->SpawnActor<AActor>(Class, FTransform::Identity, Params);
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
		Scene->AddComponent(Mesh, FTransform::Identity);
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

void FKzComboPreview::ShowItem()
{
	if (Weapon)
	{
		Weapon->Destroy();
	}
	if (WeaponMesh && Scene)
	{
		Scene->RemoveComponent(WeaponMesh);
	}
	Weapon = nullptr;
	WeaponMesh = nullptr;

	const UKzComboGraph* Asset = Combo.Get();
	ShownItem = Asset ? Asset->PreviewItem : TSoftObjectPtr<UKzItemDefinition>();
	const UKzItemDefinition* Item = ShownItem.LoadSynchronous();
	const UKzItemFragment_Equippable* Equippable = Item ? Item->FindFragmentByClass<UKzItemFragment_Equippable>() : nullptr;
	if (!Scene || !Mesh || !Equippable)
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
			Scene->AddComponent(WeaponMesh, FTransform::Identity);
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
		Weapon = Scene->GetWorld()->SpawnActor<AActor>(Class, FTransform::Identity, Params);
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

void FKzComboPreview::ShowAnimation()
{
	const float Time = Animation ? Animation->GetTime() : 0.0f;
	const bool bPlaying = !Animation || Animation->IsPlaying();
	Animation.Reset();
	Notifies.Reset();
	Trails.Reset();

	const FKzComboNode_Attack* Attack = GetSelectedAttack();
	const UKzComboGraph* Asset = Combo.Get();
	if (!Mesh || !Attack || !Attack->Animation || !Asset)
	{
		return;
	}

	FKzAnimationContext Context;
	Context.Mesh = Mesh;
	Context.User = Character;
	Context.Item = Asset->PreviewItem.LoadSynchronous();

	Animation = KzAnimationPreview::Make(*Attack->Animation, Context, *Mesh);
	if (Animation)
	{
		Attack->Animation->GetTimeline(Context, Notifies);
		Animation->SetRate(Rate);
		Animation->SetTime(FMath::Min(Time, Animation->GetLength()));
		Animation->SetPlaying(bPlaying);
		LastTime = Animation->GetTime();
	}
}

const FKzComboNode_Attack* FKzComboPreview::GetSelectedAttack() const
{
	const UKzEdGraphNode* Node = SelectedNode.Get();
	return Node ? Node->Node.GetPtr<FKzComboNode_Attack>() : nullptr;
}

const USceneComponent* FKzComboPreview::GetHitboxParent(const FKzComboHitbox& Hitbox) const
{
	return Hitbox.FindParent(Mesh, Weapon ? Weapon->GetRootComponent() : WeaponMesh.Get());
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
		return true;
	}
	SelectedHitbox = INDEX_NONE;
	return false;
}

TOptional<FTransform> FKzComboPreview::GetGizmoTransform() const
{
	const FKzComboNode_Attack* Attack = GetSelectedAttack();
	return Attack && Attack->Hitboxes.IsValidIndex(SelectedHitbox) ? GetHitboxTransform(Attack->Hitboxes[SelectedHitbox]) : TOptional<FTransform>();
}

void FKzComboPreview::MoveGizmo(const FVector& Drag, const FRotator& Rotation)
{
	UKzEdGraphNode* Node = SelectedNode.Get();
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
	Trails.Reset();
}

void FKzComboPreview::BeginGizmoDrag()
{
	GizmoTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("MoveHitbox", "Move Hitbox"));
	if (UKzEdGraphNode* Node = SelectedNode.Get())
	{
		Node->Modify();
	}
}

void FKzComboPreview::EndGizmoDrag()
{
	if (UKzEdGraphNode* Node = SelectedNode.Get())
	{
		Node->PostEditChange();
	}
	GizmoTransaction.Reset();
}

bool FKzComboPreview::IsInHitWindow(float Time) const
{
	return Notifies.ContainsByPredicate([Time](const FKzAnimationNotifySpan& Span)
	{
		return Span.Notify && Span.Notify->IsA<UKzAnimNotifyState_HitWindow>() && Time >= Span.Start && Time <= Span.End;
	});
}

void FKzComboPreview::RecordTrails()
{
	const FKzComboNode_Attack* Attack = GetSelectedAttack();
	if (!Attack || !Animation)
	{
		return;
	}

	Trails.SetNum(Attack->Hitboxes.Num());
	const float Time = Animation->GetTime();
	if (!IsInHitWindow(Time))
	{
		return;
	}

	for (int32 Index = 0; Index < Attack->Hitboxes.Num(); ++Index)
	{
		const TOptional<FTransform> Transform = GetHitboxTransform(Attack->Hitboxes[Index]);
		if (!Transform)
		{
			continue;
		}

		// One sample per frame of the animation, the latest kept, so playing the window again does not pile them up.
		TArray<FTrailSample>& Trail = Trails[Index];
		FTrailSample* Same = Trail.FindByPredicate([Time](const FTrailSample& Sample) { return FMath::Abs(Sample.Time - Time) < StepSeconds * 0.5f; });
		if (Same)
		{
			Same->Transform = *Transform;
		}
		else
		{
			Trail.Add({ Time, *Transform });
		}
	}
}

#undef LOCTEXT_NAMESPACE