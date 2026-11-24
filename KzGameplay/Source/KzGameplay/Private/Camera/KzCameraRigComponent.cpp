// Copyright 2026 kirzo

#include "Camera/KzCameraRigComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/KzCameraDirector.h"
#include "Camera/KzCameraModule.h"
#include "Camera/KzCameraVolume.h"
#include "CineCameraComponent.h"
#include "Components/KzSplineFollowerComponent.h"
#include "Components/SplineComponent.h"
#include "Core/KzRegistrySubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"

uint64 UKzCameraRigComponent::DebuggerFrame = 0;

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

	// The camera director looks here for the cameras it can switch to.
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void UKzCameraRigComponent::OnUnregister()
{
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->Unregister(this);
	}

	Super::OnUnregister();
}

void UKzCameraRigComponent::BeginPlay()
{
	Super::BeginPlay();

	// The camera director wakes the rig when it comes into view.
	Sleep();
}

void UKzCameraRigComponent::Wake()
{
	if (IsAwake()) return;

	bPendingTeleport = true;
	SetComponentTickEnabled(true);
	UpdateRig(0.0f);
}

void UKzCameraRigComponent::Sleep()
{
	SetComponentTickEnabled(false);
}

float UKzCameraRigComponent::GetDistanceTo(const FVector& Location) const
{
	const UKzSplineFollowerComponent* SplineFollower = Follower.Get();
	if (const USplineComponent* Spline = SplineFollower ? SplineFollower->GetSpline() : nullptr)
	{
		const float Distance = UKzSplineFollowerComponent::FindClosestDistance(Spline, Location, SplineFollower->IgnoredAxes);
		return FVector::Dist(Location, Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World));
	}

	const USceneComponent* Parent = GetAttachParent();
	return FVector::Dist(Location, Parent ? Parent->GetComponentLocation() : GetOwner()->GetActorLocation());
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
	for (UKzCameraModule* Module : Settings.Modules)
	{
		if (Module)
		{
			Layer.Modules.Add(Module->HasState() ? DuplicateObject(Module, this) : Module);
		}
	}

	const float BlendTime = Settings.bOverrideBlendTime ? Settings.BlendTime : DefaultBlendTime;
	if (!Layers.IsActive(Source) && GetWorld() && GetWorld()->IsGameWorld())
	{
		UKzCameraDirector::RecordEvent(this, FString::Printf(TEXT("%s: + %s, priority %d%s, %s"), *UKzCameraDirector::Describe(this), *UKzCameraDirector::Describe(Source),
			Settings.Priority, Settings.bClearPrevious ? TEXT(", clears previous") : TEXT(""), bSnap ? TEXT("at once") : *FString::Printf(TEXT("%.1f s"), BlendTime)));
	}

	Layers.Push(Source, MoveTemp(Layer), Settings.Priority, BlendTime, Settings.bClearPrevious, bSnap);
}

void UKzCameraRigComponent::PopLayer(UObject* Source, bool bSnap)
{
	const FLayer* Layer = Layers.Find(Source);
	if (Layer && !Layer->bRemoving && GetWorld() && GetWorld()->IsGameWorld())
	{
		UKzCameraDirector::RecordEvent(this, FString::Printf(TEXT("%s: - %s, %s"), *UKzCameraDirector::Describe(this), *UKzCameraDirector::Describe(Source),
			bSnap ? TEXT("at once") : *FString::Printf(TEXT("%.1f s"), Layer->BlendTime)));
	}

	Layers.Pop(Source, bSnap);
}

void UKzCameraRigComponent::SetLayerAlpha(UObject* Source, float Alpha)
{
	Layers.SetAlpha(Source, Alpha);
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

	// In real time: a world slowed down, such as by a slow motion, still has the camera follow at its own pace
	const AWorldSettings* WorldSettings = GetWorld() ? GetWorld()->GetWorldSettings(false, false) : nullptr;
	const float WorldDilation = WorldSettings ? WorldSettings->GetEffectiveTimeDilation() : 1.0f;
	if (WorldDilation > UE_SMALL_NUMBER)
	{
		DeltaTime /= WorldDilation;
	}

	UpdateRig(DeltaTime);
}

void UKzCameraRigComponent::UpdateRig(float DeltaTime)
{
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
	Layers.Update(DeltaTime, bSnap);

	// Focus: what the camera follows.
	FKzCameraPose Pose;
	Pose.Focus = Context.TargetLocation;
	Pose.FieldOfView = FieldOfView;
	EvaluateStages(EKzCameraStage::Track, EKzCameraStage::Smooth, Context, Pose, Layers.GetOrdered());

	// A driven follower takes the focus onto its spline and moves the root; the markers it crosses may add or remove layers.
	if (SplineFollower && SplineFollower->Mode == EKzSplineFollowMode::Driven && Context.Target)
	{
		SplineFollower->ProjectTo(Pose.Focus, Context.bTeleport, DeltaTime);
		Context.Anchor = Parent ? Parent->GetComponentTransform() : GetOwner()->GetActorTransform();
		Context.Reference = Context.Anchor;
	}

	const TArray<FLayer*> OrderedLayers = Layers.GetOrdered();

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
		const bool bActive = Layers.IsActive(Volume);

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
		// A volume that keeps its layer however it is left only drops it on a jump, or in the editor's preview.
		else if (bActive && (bSnap || !Volume->KeepsAfterLeaving()))
		{
			PopLayer(Volume, bVolumeSnap);
		}
	});
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
	bool bDraw = bDrawDebug || (DebuggerFrame > 0 && GFrameCounter <= DebuggerFrame + 1);
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
