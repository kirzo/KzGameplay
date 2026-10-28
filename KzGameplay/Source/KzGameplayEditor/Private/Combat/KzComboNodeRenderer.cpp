// Copyright 2026 kirzo

#include "Combat/KzComboNodeRenderer.h"
#include "AdvancedPreviewScene.h"
#include "Animation/KzAnimationPreview.h"
#include "Combat/KzCombatNotifies.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodes.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Items/KzItemDefinition.h"
#include "KzEdGraphNode.h"

namespace
{
	TMap<TWeakObjectPtr<UKzComboGraph>, TWeakPtr<FKzComboNodeRenderer>> Renderers;

	constexpr float FieldOfView = 30.0f;

	// The world moves on this much after posing, so the pose is evaluated before the capture.
	constexpr float PoseDeltaTime = 1.0f / 60.0f;
}

TSharedRef<FKzComboNodeRenderer> FKzComboNodeRenderer::Get(UKzComboGraph& Combo)
{
	if (const TSharedPtr<FKzComboNodeRenderer> Existing = Renderers.FindRef(&Combo).Pin())
	{
		return Existing.ToSharedRef();
	}

	const TSharedRef<FKzComboNodeRenderer> Renderer = MakeShared<FKzComboNodeRenderer>(Combo);
	Renderers.Add(&Combo, Renderer);
	return Renderer;
}

FKzComboNodeRenderer::FKzComboNodeRenderer(UKzComboGraph& InCombo) : Combo(&InCombo)
{
}

FKzComboNodeRenderer::~FKzComboNodeRenderer()
{
	Renderers.Remove(Combo);
}

void FKzComboNodeRenderer::Request(const UKzEdGraphNode& Node, TOptional<float> Time, UTextureRenderTarget2D& Target)
{
	Requests.Add(&Target, { &Node, Time });
}

void FKzComboNodeRenderer::Invalidate(const UKzEdGraphNode& Node)
{
	if (PreparedNode == &Node)
	{
		PreparedNode.Reset();
	}
}

void FKzComboNodeRenderer::SetSelection(const TSet<UObject*>& Selection)
{
	Selected.Reset();
	for (const UObject* Object : Selection)
	{
		Selected.Add(Object);
	}
}

bool FKzComboNodeRenderer::IsSelected(const UKzEdGraphNode& Node) const
{
	return Selected.Contains(&Node);
}

void FKzComboNodeRenderer::AddReferencedObjects(FReferenceCollector& Collector)
{
	Stage.AddReferencedObjects(Collector);
	Collector.AddReferencedObject(Capture);
}

void FKzComboNodeRenderer::Tick(float DeltaTime)
{
	if (Requests.IsEmpty())
	{
		return;
	}

	const TMap<TWeakObjectPtr<UTextureRenderTarget2D>, FRequest> Pending = MoveTemp(Requests);
	Requests.Reset();

	for (const TPair<TWeakObjectPtr<UTextureRenderTarget2D>, FRequest>& Pair : Pending)
	{
		UTextureRenderTarget2D* Target = Pair.Key.Get();
		const UKzEdGraphNode* Node = Pair.Value.Node.Get();
		if (!Target || !Node)
		{
			continue;
		}

		if (!Prepare(*Node))
		{
			continue;
		}

		// Unset, the middle of the first Hit Window, where the swing shows best.
		const float Length = Animation->GetLength();
		float Time = Length * 0.4f;
		if (Pair.Value.Time)
		{
			Time = Length > 0.0f ? FMath::Fmod(*Pair.Value.Time, Length) : 0.0f;
		}
		else if (const FKzAnimationNotifySpan* HitWindow = Notifies.FindByPredicate([](const FKzAnimationNotifySpan& Span) { return Span.Notify && Span.Notify->IsA<UKzAnimNotifyState_HitWindow>(); }))
		{
			Time = (HitWindow->Start + HitWindow->End) * 0.5f;
		}

		Animation->SetTime(Time);
		Scene->GetWorld()->Tick(LEVELTICK_All, PoseDeltaTime);
		Capture->TextureTarget = Target;
		Capture->CaptureScene();
	}
}

TStatId FKzComboNodeRenderer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FKzComboNodeRenderer, STATGROUP_Tickables);
}

bool FKzComboNodeRenderer::Prepare(const UKzEdGraphNode& Node)
{
	UKzComboGraph* Asset = Combo.Get();
	const FKzComboNode_Attack* Attack = Node.Node.GetPtr<FKzComboNode_Attack>();
	if (!Asset || !Attack || !Attack->Animation)
	{
		return false;
	}

	if (!Scene)
	{
		Scene = MakeUnique<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
		Scene->GetWorld()->GetWorldSettings()->MinUndilatedFrameTime = 0.0f;

		// Captured on demand only, without the temporal effects a single frame cannot settle.
		Capture = NewObject<USceneCaptureComponent2D>(GetTransientPackage(), NAME_None, RF_Transient);
		Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Capture->bCaptureEveryFrame = false;
		Capture->bCaptureOnMovement = false;
		Capture->bAlwaysPersistRenderingState = true;
		Capture->FOVAngle = FieldOfView;
		Capture->ShowFlags.SetTemporalAA(false);
		Capture->ShowFlags.SetMotionBlur(false);
		Scene->AddComponent(Capture, FTransform::Identity);
	}

	if (Stage.Update(*Scene, *Asset))
	{
		PreparedNode.Reset();
		Animation.Reset();
		Frame();
	}

	USkeletalMeshComponent* Mesh = Stage.GetMesh();
	if (!Mesh)
	{
		return false;
	}

	if (PreparedNode != &Node || !Animation)
	{
		FKzAnimationContext Context;
		Context.Mesh = Mesh;
		Context.User = Stage.GetCharacter();
		Context.Item = Asset->PreviewItem.LoadSynchronous();

		Notifies.Reset();
		Animation = KzAnimationPreview::Make(*Attack->Animation, Context, *Mesh);
		if (Animation)
		{
			Attack->Animation->GetTimeline(Context, Notifies);
			Animation->SetPlaying(false);
		}
		PreparedNode = &Node;
	}
	return Animation.IsValid();
}

void FKzComboNodeRenderer::Frame()
{
	const USkeletalMeshComponent* Mesh = Stage.GetMesh();
	if (!Mesh || !Capture)
	{
		return;
	}

	// The character's bounding sphere fits the height of the picture, with room for the swing.
	const FBoxSphereBounds Bounds = Mesh->CalcBounds(Mesh->GetComponentTransform());
	const float HalfHeightAngle = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f)) * static_cast<float>(Height) / Width);
	const float Distance = Bounds.SphereRadius * 1.1f / FMath::Sin(HalfHeightAngle);
	const FVector Direction = FVector(1.0, 0.6, 0.3).GetSafeNormal();
	Capture->SetWorldLocationAndRotation(Bounds.Origin + Direction * Distance, (-Direction).Rotation());
}