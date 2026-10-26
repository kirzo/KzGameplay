// Copyright 2026 kirzo

#include "Combat/KzComboPreview.h"
#include "AdvancedPreviewScene.h"
#include "Animation/KzAnimationNotifyState.h"
#include "Animation/KzAnimationPreview.h"
#include "CanvasTypes.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Equipment/KzEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "Items/Fragments/KzItemFragment_Equippable.h"
#include "Items/KzItemDefinition.h"
#include "KzEdGraphNode.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KzComboPreview"

namespace
{
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
			return FVector2D(100.0, 22.0);
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

			for (const FKzAnimationNotifySpan& Span : Pinned->GetNotifies())
			{
				const float Start = Size.X * FMath::Clamp(Span.Start / Length, 0.0f, 1.0f);
				const float End = Size.X * FMath::Clamp(Span.End / Length, 0.0f, 1.0f);
				const FLinearColor Color = Span.Notify ? FLinearColor(Span.Notify->NotifyColor) : FLinearColor::Gray;
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(FMath::Max(End - Start, 2.0f), Size.Y - 6.0f), FSlateLayoutTransform(FVector2f(Start, 3.0f))), White, ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.7f));
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
			ScrubTo(MyGeometry, MouseEvent);
			return FReply::Handled().CaptureMouse(SharedThis(this));
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
	const TSharedRef<FKzComboPreview> Self = StaticCastSharedRef<FKzComboPreview>(AsShared());
	const TWeakPtr<FKzComboPreview> Weak = Self;

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("PlayTooltip", "Plays or pauses the attack."))
				.OnClicked_Lambda([Weak]()
				{
					if (const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin())
					{
						Pinned->TogglePlay();
					}
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image_Lambda([Weak]()
					{
						const TSharedPtr<FKzComboPreview> Pinned = Weak.Pin();
						return FAppStyle::GetBrush(Pinned && Pinned->IsPlaying() ? "Icons.Pause" : "Icons.Play");
					})
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SKzComboTrack, Self)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
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
		}
	}
}

void FKzComboPreview::ShowAnimation()
{
	const float Time = Animation ? Animation->GetTime() : 0.0f;
	const bool bPlaying = !Animation || Animation->IsPlaying();
	Animation.Reset();
	Notifies.Reset();

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
		Animation->SetTime(FMath::Min(Time, Animation->GetLength()));
		Animation->SetPlaying(bPlaying);
	}
}

const FKzComboNode_Attack* FKzComboPreview::GetSelectedAttack() const
{
	const UKzEdGraphNode* Node = SelectedNode.Get();
	return Node ? Node->Node.GetPtr<FKzComboNode_Attack>() : nullptr;
}

#undef LOCTEXT_NAMESPACE