// Copyright 2026 kirzo

#include "Camera/KzCameraRigComponent.h"
#include "Algo/Reverse.h"
#include "Camera/CameraComponent.h"
#include "Camera/KzCameraModule.h"
#include "Camera/KzCameraVolume.h"
#include "CineCameraComponent.h"
#include "Components/KzSplineFollowerComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UKzCameraRigComponent::UKzCameraRigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bTickInEditor = true;
	bAutoActivate = true;
}

void UKzCameraRigComponent::OnRegister()
{
	Super::OnRegister();

	bPendingTeleport = true;
	Camera.Reset();

	// A follower that moves on its own leaves the root before the rig hangs from it.
	Follower = GetOwner() ? GetOwner()->FindComponentByClass<UKzSplineFollowerComponent>() : nullptr;
	if (Follower.IsValid())
	{
		AddTickPrerequisiteComponent(Follower.Get());
	}
}

void UKzCameraRigComponent::BeginPlay()
{
	Super::BeginPlay();

	bPendingTeleport = true;

	if (bAutoActivateForPlayer)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			PlayerController->SetViewTarget(GetOwner());
		}
	}
}

void UKzCameraRigComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	// Layer modules live outside any UPROPERTY: shared ones belong to their source, stateful copies to the rig.
	CastChecked<UKzCameraRigComponent>(InThis)->Layers.ForEach([&Collector](FLayer& Layer)
	{
		Collector.AddReferencedObjects(Layer.Modules);
	});
}

void UKzCameraRigComponent::PushLayer(UObject* Source, const FKzCameraLayerSettings& Settings, bool bSnap)
{
	if (!Source) return;

	FLayer Layer;

	// Pushed again while blending out: it blends back in from where it is.
	if (const FLayer* Existing = Layers.Find(Source))
	{
		Layer.Alpha = Existing->Alpha;
		Layer.Blend = Existing->Blend;
	}

	Layer.Source = Source;
	Layer.BlendTime = Settings.bOverrideBlendTime ? Settings.BlendTime : DefaultBlendTime;
	Layer.bClearPrevious = Settings.bClearPrevious;
	Layer.Order = ++LastLayerOrder;

	if (bSnap)
	{
		Layer.Blend = 1.0f;
	}

	for (UKzCameraModule* Module : Settings.Modules)
	{
		if (Module)
		{
			Layer.Modules.Add(Module->HasState() ? DuplicateObject(Module, this) : Module);
		}
	}

	Layers.Push(Layer, Source, Settings.Priority);
}

void UKzCameraRigComponent::PopLayer(UObject* Source, bool bSnap)
{
	if (FLayer* Layer = Layers.Find(Source))
	{
		Layer->bRemoving = true;
		if (bSnap)
		{
			Layer->Blend = 0.0f;
		}
	}
}

void UKzCameraRigComponent::SetLayerAlpha(UObject* Source, float Alpha)
{
	if (FLayer* Layer = Layers.Find(Source))
	{
		Layer->Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	}
}

AActor* UKzCameraRigComponent::GetTarget() const
{
	const UWorld* World = GetWorld();
	if (bTargetPlayerPawn && World && World->IsGameWorld())
	{
		const APlayerController* PlayerController = World->GetFirstPlayerController();
		return PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	return Target;
}

UCameraComponent* UKzCameraRigComponent::FindCamera()
{
	if (!Camera.IsValid())
	{
		TArray<USceneComponent*> Children;
		GetChildrenComponents(true, Children);

		for (USceneComponent* Child : Children)
		{
			if (UCameraComponent* Found = Cast<UCameraComponent>(Child))
			{
				Camera = Found;
				break;
			}
		}
	}

	return Camera.Get();
}

void UKzCameraRigComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UCameraComponent* CameraComponent = FindCamera();
	UKzSplineFollowerComponent* SplineFollower = Follower.Get();
	const USceneComponent* Parent = GetAttachParent();

	FKzCameraContext Context;
	Context.Rig = this;
	Context.Follower = SplineFollower;
	Context.Target = GetTarget();
	Context.DeltaTime = DeltaTime;
	Context.CameraRelative = CameraComponent ? CameraComponent->GetRelativeTransform() : FTransform::Identity;
	Context.Anchor = Parent ? Parent->GetComponentTransform() : GetOwner()->GetActorTransform();
	Context.Reference = Context.Anchor;
	Context.Rest = LastRest;
	Context.LastView = LastView;
	Context.TargetLocation = Context.Target ? Context.Target->GetActorLocation() : Context.Anchor.GetLocation();

	// A target that changes or jumps too far teleports the camera.
	const bool bTargetJumped = Context.Target.Get() != LastTarget.Get()
		|| (TeleportDistance > 0.0f && FVector::DistSquared(Context.TargetLocation, LastTargetLocation) > FMath::Square(TeleportDistance));
	Context.bTeleport = bPendingTeleport || bTargetJumped;
	bPendingTeleport = false;
	LastTarget = Context.Target.Get();
	LastTargetLocation = Context.TargetLocation;

	// The editor previews where the camera settles, without time blends.
	const bool bSnap = Context.bTeleport || !GetWorld()->IsGameWorld();

	if (Context.Target)
	{
		UpdateVolumes(Context.TargetLocation, bSnap);
	}
	UpdateLayerBlends(DeltaTime, bSnap);

	// Focus: what the camera follows.
	FKzCameraPose Pose;
	Pose.Focus = Context.TargetLocation;
	Pose.FieldOfView = FieldOfView;
	EvaluateStages(EKzCameraStage::Track, EKzCameraStage::Smooth, Context, Pose, GetOrderedLayers());

	// A driven follower takes the focus onto its spline and moves the root; the markers it crosses may add or remove layers.
	if (SplineFollower && SplineFollower->Mode == EKzSplineFollowMode::Driven && Context.Target)
	{
		SplineFollower->ProjectTo(Pose.Focus, Context.bTeleport, DeltaTime);
		Context.Anchor = Parent ? Parent->GetComponentTransform() : GetOwner()->GetActorTransform();
		Context.Reference = Context.Anchor;
	}

	const TArray<FLayer*> OrderedLayers = GetOrderedLayers();

	// Anchor: where the camera hangs from, keeping its authored offset.
	const FTransform HangingTransform = Context.CameraRelative * Context.Anchor;
	Pose.Location = HangingTransform.GetLocation();
	Pose.Rotation = HangingTransform.Rotator();
	EvaluateStages(EKzCameraStage::Anchor, EKzCameraStage::Anchor, Context, Pose, OrderedLayers);

	// The point the anchor modules left the camera hanging from, and where it points from there before aiming.
	Context.Reference = Context.CameraRelative.Inverse() * FTransform(Pose.Rotation, Pose.Location);
	Context.Rest = Pose.Rotation;

	EvaluateStages(EKzCameraStage::Position, EKzCameraStage::Lens, Context, Pose, OrderedLayers);
	ApplyPose(Pose, CameraComponent);

	LastPose = Pose;
	LastRest = Context.Rest;
	LastView.Location = Pose.Location;
	LastView.Rotation = Pose.Rotation;
	LastView.FOV = Pose.FieldOfView;
	LastView.AspectRatio = CameraComponent ? CameraComponent->AspectRatio : 16.0f / 9.0f;

	DrawDebug(Context, Pose, OrderedLayers);
}

void UKzCameraRigComponent::UpdateVolumes(const FVector& TargetLocation, bool bSnap)
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return;

	Registry->ForEach<AKzCameraVolume>([&](AKzCameraVolume* Volume)
	{
		const float Alpha = Volume->GetAlpha(TargetLocation);
		const FLayer* Layer = Layers.Find(Volume);
		const bool bActive = Layer && !Layer->bRemoving;

		// A progressive volume already blends in space, so it adds no time blend on top.
		const bool bVolumeSnap = bSnap || Volume->bProgressive;

		if (Alpha > 0.0f)
		{
			if (!bActive)
			{
				PushLayer(Volume, Volume->Layer, bVolumeSnap);
			}
			SetLayerAlpha(Volume, Alpha);
		}
		else if (bActive)
		{
			PopLayer(Volume, bVolumeSnap);
		}
	});
}

void UKzCameraRigComponent::UpdateLayerBlends(float DeltaTime, bool bSnap)
{
	Layers.ForEach([DeltaTime, bSnap](FLayer& Layer)
	{
		const float TargetBlend = Layer.bRemoving ? 0.0f : 1.0f;
		Layer.Blend = bSnap || Layer.BlendTime <= 0.0f ? TargetBlend : FMath::FInterpConstantTo(Layer.Blend, TargetBlend, DeltaTime, 1.0f / Layer.BlendTime);
	});

	Layers.RemoveAll([](const FLayer& Layer)
	{
		return !Layer.Source.IsValid() || (Layer.bRemoving && Layer.Blend <= 0.0f);
	});
}

TArray<UKzCameraRigComponent::FLayer*> UKzCameraRigComponent::GetOrderedLayers()
{
	TArray<FLayer*> OrderedLayers = Layers.GetSortedElements();
	Algo::Reverse(OrderedLayers);
	UpdateLayerWeights(OrderedLayers);
	return OrderedLayers;
}

void UKzCameraRigComponent::UpdateLayerWeights(const TArray<FLayer*>& InLayers)
{
	TArray<FLayer*> NewestFirst = InLayers;
	NewestFirst.Sort([](const FLayer& A, const FLayer& B) { return A.Order > B.Order; });

	float Visible = 1.0f;
	for (FLayer* Layer : NewestFirst)
	{
		const float OwnWeight = Layer->Alpha * FMath::SmoothStep(0.0f, 1.0f, Layer->Blend);
		Layer->Weight = OwnWeight * Visible;

		if (Layer->bClearPrevious)
		{
			Visible *= 1.0f - OwnWeight;
		}
	}
}

void UKzCameraRigComponent::EvaluateStages(EKzCameraStage First, EKzCameraStage Last, const FKzCameraContext& Context, FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers) const
{
	struct FSlot
	{
		UClass* Class;
		EKzCameraStage Stage;
	};

	TArray<FSlot, TInlineAllocator<16>> Slots;
	auto AddSlots = [&Slots, First, Last](const TArray<TObjectPtr<UKzCameraModule>>& InModules)
	{
		for (const UKzCameraModule* Module : InModules)
		{
			if (!Module) continue;

			const EKzCameraStage Stage = Module->GetStage();
			if (Stage < First || Stage > Last) continue;

			UClass* SlotClass = Module->GetSlotClass();
			if (!Slots.ContainsByPredicate([SlotClass](const FSlot& Slot) { return Slot.Class == SlotClass; }))
			{
				Slots.Add({ SlotClass, Stage });
			}
		}
	};

	AddSlots(Modules);
	for (const FLayer* Layer : OrderedLayers)
	{
		AddSlots(Layer->Modules);
	}

	Slots.StableSort([](const FSlot& A, const FSlot& B) { return A.Stage < B.Stage; });

	for (const FSlot& Slot : Slots)
	{
		Pose = EvaluateSlot(Slot.Class, Context, Pose, OrderedLayers);
	}
}

FKzCameraPose UKzCameraRigComponent::EvaluateSlot(const UClass* SlotClass, const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers) const
{
	auto FindModule = [SlotClass](const TArray<TObjectPtr<UKzCameraModule>>& InModules) -> UKzCameraModule*
	{
		for (UKzCameraModule* Module : InModules)
		{
			if (Module && Module->GetSlotClass() == SlotClass)
			{
				return Module;
			}
		}
		return nullptr;
	};

	// Every module of the kind works from the same input; the layers above blend over the ones below.
	UKzCameraModule* BaseModule = FindModule(Modules);
	FKzCameraPose Out = BaseModule ? BaseModule->Evaluate(Context, Pose) : Pose;

	for (const FLayer* Layer : OrderedLayers)
	{
		if (Layer->Weight <= 0.0f) continue;

		if (UKzCameraModule* Module = FindModule(Layer->Modules))
		{
			Out = FKzCameraPose::Blend(Out, Module->Evaluate(Context, Pose), Layer->Weight);
		}
	}

	return Out;
}

void UKzCameraRigComponent::ApplyPose(const FKzCameraPose& Pose, UCameraComponent* CameraComponent)
{
	// The pose is the camera's; the rig takes it minus the camera's own offset, which stays as authored.
	const FTransform CameraRelative = CameraComponent ? CameraComponent->GetRelativeTransform() : FTransform::Identity;
	const FTransform RigTransform = CameraRelative.Inverse() * FTransform(Pose.Rotation, Pose.Location);
	SetWorldLocationAndRotation(RigTransform.GetLocation(), RigTransform.GetRotation());

	if (!CameraComponent) return;

	CameraComponent->SetFieldOfView(Pose.FieldOfView);

	UCineCameraComponent* CineCamera = Cast<UCineCameraComponent>(CameraComponent);
	if (CineCamera && Pose.FocusDistance > 0.0f)
	{
		CineCamera->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
		CineCamera->FocusSettings.ManualFocusDistance = Pose.FocusDistance;
		CineCamera->CurrentAperture = Pose.Aperture;
	}
}

void UKzCameraRigComponent::DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers) const
{
	bool bDraw = bDrawDebug;
#if WITH_EDITOR
	bDraw |= !GetWorld()->IsGameWorld() && GetOwner()->IsSelectedInEditor();
#endif
	if (!bDraw) return;

	// The target, the focus and the line between them, which is how far behind or ahead the camera follows.
	constexpr float Thickness = 1.5f;
	const UWorld* World = GetWorld();
	DrawDebugSphere(World, Context.TargetLocation, 12.0f, 8, FColor::Red, false, -1.0f, SDPG_Foreground, Thickness);
	DrawDebugSphere(World, Pose.Focus, 18.0f, 8, FColor::Yellow, false, -1.0f, SDPG_Foreground, Thickness);
	DrawDebugLine(World, Context.TargetLocation, Pose.Focus, FColor::Yellow, false, -1.0f, SDPG_Foreground, Thickness);

	for (const UKzCameraModule* Module : Modules)
	{
		if (Module)
		{
			Module->DrawDebug(Context, Pose);
		}
	}

	for (const FLayer* Layer : OrderedLayers)
	{
		if (Layer->Weight <= 0.0f) continue;

		for (const UKzCameraModule* Module : Layer->Modules)
		{
			if (Module)
			{
				Module->DrawDebug(Context, Pose);
			}
		}
	}
}
