// Copyright 2026 kirzo

#include "Camera/KzCameraRigComponent.h"
#include "Actors/KzStandIn.h"
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

void UKzCameraRigComponent::Wake(bool bTargetJumped)
{
	if (IsAwake()) return;

	bPendingTeleport = true;
	if (bTargetJumped)
	{
		// Read as a new target, so the volumes apply by where it is.
		LastTarget.Reset();
	}
	else
	{
		// The camera only comes into view: the target didn't jump while the rig slept, and the volumes it is in belong to the camera that saw it enter.
		LastTarget = GetTarget();
		LastTargetLocation = LastTarget.IsValid() ? LastTarget->GetTargetLocation() : LastTargetLocation;
		bWokeClean = true;
	}

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

	if (Target || !World || World->IsGameWorld())
	{
		return Target;
	}

	// In the editor without a target, the stand-in nearest where the camera hangs, so one stand-in previews every camera.
	AActor* Nearest = nullptr;
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		float NearestDistance = TNumericLimits<float>::Max();
		Registry->ForEach<AKzStandIn>([this, &Nearest, &NearestDistance](AKzStandIn* StandIn)
		{
			const float Distance = GetDistanceTo(StandIn->GetTargetLocation());
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = StandIn;
			}
		});
	}
	return Nearest;
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
	// Where to follow the target: its location, or for a stand-in the middle of the character it stands for.
	Context.TargetLocation = Context.Target ? Context.Target->GetTargetLocation() : Context.Anchor.GetLocation();

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
		UpdateVolumes(Context.TargetLocation, bSnap, bTargetJumped || !GetWorld()->IsGameWorld());
	}
	Layers.Update(DeltaTime, bSnap);

	TArray<FDebugSlot> DebugSlots;
	TArray<FDebugSlot>* Debug = ShouldDrawDebug() ? &DebugSlots : nullptr;

	// Focus: what the camera follows.
	FKzCameraPose Pose;
	Pose.Focus = Context.TargetLocation;
	Pose.FieldOfView = FieldOfView;
	EvaluateStages(EKzCameraStage::Track, EKzCameraStage::Smooth, Context, Pose, Layers.GetOrdered(), Debug);

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
	EvaluateStages(EKzCameraStage::Anchor, EKzCameraStage::Anchor, Context, Pose, OrderedLayers, Debug);

	// The point the anchor modules left the camera hanging from, and where it points from there before aiming.
	Context.Reference = Context.CameraRelative.Inverse() * FTransform(Pose.Rotation, Pose.Location);
	Context.Rest = Pose.Rotation;

	EvaluateStages(EKzCameraStage::Position, EKzCameraStage::Lens, Context, Pose, OrderedLayers, Debug);
	ApplyPose(Pose, CameraComponent);
	UpdateStandInPreview(Cast<AKzStandIn>(Context.Target), CameraComponent);

	LastPose = Pose;
	LastRest = Context.Rest;
	LastView.Location = Pose.Location;
	LastView.Rotation = Pose.Rotation;
	LastView.FOV = Pose.FieldOfView;
	LastView.AspectRatio = CameraComponent ? CameraComponent->AspectRatio : 16.0f / 9.0f;

	if (Debug)
	{
		DrawDebug(Context, Pose, DebugSlots);
	}
}

void UKzCameraRigComponent::UpdateVolumes(const FVector& TargetLocation, bool bSnap, bool bReread)
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return;

	// Reread after a jump (start, respawn) or in the editor's preview: the volumes apply by where the target is, and nothing else.
	const bool bWoke = bWokeClean && !bReread;
	bWokeClean = false;
	if (bReread)
	{
		IgnoredVolumes.Reset();
	}

	Registry->ForEach<AKzCameraVolume>([&](AKzCameraVolume* Volume)
	{
		const float Alpha = Volume->GetAlpha(TargetLocation);
		const bool bActive = Layers.IsActive(Volume);
		const bool bInside = Volume->Contains(TargetLocation);

		// A progressive volume already blends in space, so it adds no time blend on top.
		const bool bVolumeSnap = bSnap || Volume->bProgressive;

		// Woken into view, the rig leaves the volumes the target is inside to the camera that saw it enter, until it walks out of them.
		if (!bInside)
		{
			IgnoredVolumes.Remove(Volume);
		}
		else if (bWoke && !bActive)
		{
			IgnoredVolumes.Add(Volume);
		}

		if (bActive)
		{
			if (Alpha > 0.0f)
			{
				SetLayerAlpha(Volume, Alpha);
			}
			// Walked out past its exit, a volume keeps its layer; on a reread, the alpha alone decides.
			else if (bReread || !Volume->Keeps(TargetLocation))
			{
				PopLayer(Volume, bVolumeSnap);
			}
		}
		// Walking, a layer comes in by going inside the volume, not by reaching past its exit some other way; on a reread, by the alpha alone.
		else if (Alpha > 0.0f && (bReread || (bInside && !IgnoredVolumes.Contains(Volume))))
		{
			PushLayer(Volume, Volume->Layer, bVolumeSnap);
			SetLayerAlpha(Volume, Alpha);
		}
	});
}

void UKzCameraRigComponent::EvaluateStages(EKzCameraStage First, EKzCameraStage Last, const FKzCameraContext& Context, FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers, TArray<FDebugSlot>* DebugSlots) const
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
		const FKzCameraPose In = Pose;
		const UKzCameraModule* Top = nullptr;
		Pose = EvaluateSlot(Slot.Class, Context, Pose, OrderedLayers, Top);

		if (DebugSlots && Top)
		{
			DebugSlots->Add({ Top, In, Pose });
		}
	}
}

FKzCameraPose UKzCameraRigComponent::EvaluateSlot(const UClass* SlotClass, const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FLayer*>& OrderedLayers, const UKzCameraModule*& OutTop) const
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
	OutTop = BaseModule;

	for (const FLayer* Layer : OrderedLayers)
	{
		if (Layer->Weight <= 0.0f) continue;

		if (UKzCameraModule* Module = FindModule(Layer->Modules))
		{
			Out = FKzCameraPose::Blend(Out, Module->Evaluate(Context, Pose), Layer->Weight);
			OutTop = Module;
		}
	}

	return Out;
}

void UKzCameraRigComponent::UpdateStandInPreview(AKzStandIn* StandIn, UCameraComponent* CameraComponent) const
{
	if (!StandIn || !CameraComponent || GetWorld()->IsGameWorld()) return;

	// The camera that would show the stand-in in game, worked out once a frame by the first rig that follows it.
	static uint64 ChoiceFrame = 0;
	static TWeakObjectPtr<const AKzStandIn> ChoiceStandIn;
	static TWeakObjectPtr<const AActor> ChoiceCamera;
	if (ChoiceFrame != GFrameCounter || ChoiceStandIn.Get() != StandIn)
	{
		ChoiceFrame = GFrameCounter;
		ChoiceStandIn = StandIn;
		ChoiceCamera = UKzCameraDirector::FindCameraFor(this, StandIn->GetTargetLocation());
	}

	if (ChoiceCamera.Get() == GetOwner())
	{
		FMinimalViewInfo View;
		CameraComponent->GetCameraView(0.0f, View);
		StandIn->SetPreviewView(View);
	}
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

bool UKzCameraRigComponent::ShouldDrawDebug() const
{
	bool bDraw = bDrawDebug || (DebuggerFrame > 0 && GFrameCounter <= DebuggerFrame + 1);
#if WITH_EDITOR
	bDraw |= !GetWorld()->IsGameWorld() && GetOwner()->IsSelectedInEditor();
#endif
	return bDraw;
}

void UKzCameraRigComponent::DrawDebug(const FKzCameraContext& Context, const FKzCameraPose& Pose, const TArray<FDebugSlot>& DebugSlots) const
{
	// The target, the focus and the line between them, which is how far behind or ahead the camera follows.
	constexpr float Thickness = 1.5f;
	const UWorld* World = GetWorld();
	DrawDebugSphere(World, Context.TargetLocation, 12.0f, 8, FColor::Red, false, -1.0f, SDPG_Foreground, Thickness);
	DrawDebugSphere(World, Pose.Focus, 18.0f, 8, FColor::Yellow, false, -1.0f, SDPG_Foreground, Thickness);
	DrawDebugLine(World, Context.TargetLocation, Pose.Focus, FColor::Yellow, false, -1.0f, SDPG_Foreground, Thickness);

	// Each kind of module as its layers blend it, drawn once by the top one.
	for (const FDebugSlot& Slot : DebugSlots)
	{
		Slot.Module->DrawDebug(Context, Slot.In, Slot.Out);
	}
}
