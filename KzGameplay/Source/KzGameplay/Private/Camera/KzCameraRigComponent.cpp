// Copyright 2026 kirzo

#include "Camera/KzCameraRigComponent.h"
#include "Algo/Reverse.h"
#include "Camera/CameraComponent.h"
#include "Camera/KzCameraModule.h"
#include "Camera/KzCameraVolume.h"
#include "CineCameraComponent.h"
#include "Components/KzSplineFollowerComponent.h"
#include "Components/SplineComponent.h"
#include "Core/KzRegistrySubsystem.h"
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

	// The rig hangs from where the follower leaves the root, so it goes after it.
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
	const UKzSplineFollowerComponent* SplineFollower = Follower.Get();
	if (SplineFollower && SplineFollower->Mode == EKzSplineFollowMode::Projection)
	{
		return SplineFollower->GetProjectedActor();
	}

	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World && World->IsGameWorld() ? World->GetFirstPlayerController() : nullptr;
	return PlayerController ? PlayerController->GetPawn() : nullptr;
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

	FKzCameraContext Context;
	Context.Rig = this;
	Context.Follower = Follower.Get();
	Context.Target = GetTarget();
	Context.DeltaTime = DeltaTime;
	Context.bTeleport = bPendingTeleport || (Context.Follower && Context.Follower->WasTeleported());
	bPendingTeleport = false;

	// The editor previews where the camera settles, without time blends.
	const bool bSnap = Context.bTeleport || !GetWorld()->IsGameWorld();

	if (Context.Target)
	{
		UpdateVolumes(Context.Target->GetActorLocation(), bSnap);
	}
	UpdateLayerBlends(DeltaTime, bSnap);

	TArray<FLayer*> OrderedLayers = Layers.GetSortedElements();
	Algo::Reverse(OrderedLayers);
	UpdateLayerWeights(OrderedLayers);

	const FTransform CameraRelative = CameraComponent ? CameraComponent->GetRelativeTransform() : FTransform::Identity;
	ApplyPose(EvaluatePose(Context, CameraRelative, OrderedLayers), CameraComponent);
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
		const float Target = Layer.bRemoving ? 0.0f : 1.0f;
		Layer.Blend = bSnap || Layer.BlendTime <= 0.0f ? Target : FMath::FInterpConstantTo(Layer.Blend, Target, DeltaTime, 1.0f / Layer.BlendTime);
	});

	Layers.RemoveAll([](const FLayer& Layer)
	{
		return !Layer.Source.IsValid() || (Layer.bRemoving && Layer.Blend <= 0.0f);
	});
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

FKzCameraPose UKzCameraRigComponent::EvaluatePose(FKzCameraContext& Context, const FTransform& CameraRelative, const TArray<FLayer*>& OrderedLayers) const
{
	const USceneComponent* Parent = GetAttachParent();
	Context.Reference = Parent ? Parent->GetComponentTransform() : GetOwner()->GetActorTransform();
	Context.ProjectedLocation = Context.Reference.GetLocation();

	FKzCameraPose Pose;
	Pose.FieldOfView = FieldOfView;

	if (const UKzSplineFollowerComponent* SplineFollower = Context.Follower)
	{
		Pose.RailDistance = SplineFollower->GetCurrentDistance();
		if (const USplineComponent* Spline = SplineFollower->GetSpline())
		{
			Context.ProjectedLocation = Spline->GetLocationAtDistanceAlongSpline(Pose.RailDistance, ESplineCoordinateSpace::World);
		}
	}

	const FTransform CameraTransform = CameraRelative * Context.Reference;
	Pose.Location = CameraTransform.GetLocation();
	Pose.Rotation = CameraTransform.Rotator();

	// Every kind of module present, run by stage and, within a stage, in the order they first appear.
	struct FSlot
	{
		UClass* Class;
		EKzCameraStage Stage;
	};

	TArray<FSlot, TInlineAllocator<16>> Slots;
	auto AddSlots = [&Slots](const TArray<TObjectPtr<UKzCameraModule>>& InModules)
	{
		for (const UKzCameraModule* Module : InModules)
		{
			if (!Module) continue;

			UClass* SlotClass = Module->GetSlotClass();
			if (!Slots.ContainsByPredicate([SlotClass](const FSlot& Slot) { return Slot.Class == SlotClass; }))
			{
				Slots.Add({ SlotClass, Module->GetStage() });
			}
		}
	};

	AddSlots(Modules);
	for (const FLayer* Layer : OrderedLayers)
	{
		AddSlots(Layer->Modules);
	}

	Slots.StableSort([](const FSlot& A, const FSlot& B) { return A.Stage < B.Stage; });

	bool bOnRail = false;
	for (const FSlot& Slot : Slots)
	{
		if (!bOnRail && Slot.Stage > EKzCameraStage::Rail)
		{
			PlaceOnRail(Context, Pose, CameraRelative);
			bOnRail = true;
		}

		Pose = EvaluateSlot(Slot.Class, Context, Pose, OrderedLayers);
	}

	if (!bOnRail)
	{
		PlaceOnRail(Context, Pose, CameraRelative);
	}

	return Pose;
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

void UKzCameraRigComponent::PlaceOnRail(FKzCameraContext& Context, FKzCameraPose& Pose, const FTransform& CameraRelative) const
{
	if (const UKzSplineFollowerComponent* SplineFollower = Context.Follower)
	{
		Context.Reference = SplineFollower->GetTransformAtDistance(Pose.RailDistance);

		const FTransform CameraTransform = CameraRelative * Context.Reference;
		Pose.Location = CameraTransform.GetLocation();
		Pose.Rotation = CameraTransform.Rotator();
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
