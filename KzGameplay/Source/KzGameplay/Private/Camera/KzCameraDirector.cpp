// Copyright 2026 kirzo

#include "Camera/KzCameraDirector.h"
#include "Camera/KzCameraRigComponent.h"
#include "Camera/KzCameraZone.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UKzCameraDirector* UKzCameraDirector::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UKzCameraDirector>() : nullptr;
}

bool UKzCameraDirector::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UKzCameraDirector::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzCameraDirector, STATGROUP_Tickables);
}

APlayerController* UKzCameraDirector::GetPlayerController() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

void UKzCameraDirector::SwitchTo(AActor* Camera, const FViewTargetTransitionParams& Blend)
{
	APlayerController* PlayerController = GetPlayerController();
	if (!Camera || !PlayerController) return;

	// Awake and in place before the blend reads its view.
	if (UKzCameraRigComponent* Rig = Camera->FindComponentByClass<UKzCameraRigComponent>())
	{
		GameplayCamera = Camera;
		Rig->Wake();
	}

	PlayerController->SetViewTarget(Camera, Blend);
}

void UKzCameraDirector::ReturnToGameplay(const FViewTargetTransitionParams& Blend)
{
	SwitchTo(GameplayCamera.Get(), Blend);
}

void UKzCameraDirector::SelectCamera()
{
	const APlayerController* PlayerController = GetPlayerController();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn) return;

	const FVector Location = Pawn->GetActorLocation();
	const AKzCameraZone* Zone = FindZone(Location);
	CurrentZone = Zone;

	AActor* Camera = Zone && Zone->Camera ? Zone->Camera.Get() : FindNearestCamera(Location);
	SwitchTo(Camera, FViewTargetTransitionParams());
}

void UKzCameraDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PlayerController = GetPlayerController();
	const APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager.Get() : nullptr;
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!CameraManager || !Pawn) return;

	const FVector Location = Pawn->GetActorLocation();
	const bool bJumped = Pawn != LastPawn.Get()
		|| (TeleportDistance > 0.0f && FVector::DistSquared(Location, LastPawnLocation) > FMath::Square(TeleportDistance));
	LastPawn = Pawn;
	LastPawnLocation = Location;

	if (bJumped)
	{
		// Play start, a respawn or a teleport: cut to the camera for where the player is now.
		SelectCamera();
	}
	else
	{
		// Entering a zone switches to its camera; leaving every zone keeps the current one.
		const AKzCameraZone* Zone = FindZone(Location);
		if (Zone && Zone != CurrentZone.Get() && Zone->Camera && Zone->Camera != GameplayCamera.Get())
		{
			if (IsShowingGameplay(CameraManager))
			{
				SwitchTo(Zone->Camera, Zone->Blend);
			}
			else
			{
				// A cutscene or a fixed shot is in view: returning to gameplay will go to the zone's camera.
				GameplayCamera = Zone->Camera.Get();
			}
		}
		CurrentZone = Zone;
	}

	UpdateRigs(CameraManager);
}

bool UKzCameraDirector::IsShowingGameplay(const APlayerCameraManager* CameraManager) const
{
	const AActor* Gameplay = GameplayCamera.Get();
	return Gameplay && (CameraManager->GetViewTarget() == Gameplay || CameraManager->PendingViewTarget.Target == Gameplay);
}

void UKzCameraDirector::UpdateRigs(const APlayerCameraManager* CameraManager) const
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return;

	// Gathered first: waking a rig updates it, and its update walks the registry too.
	TArray<UKzCameraRigComponent*, TInlineAllocator<8>> Rigs;
	Registry->ForEach<UKzCameraRigComponent>([&Rigs](UKzCameraRigComponent* Rig) { Rigs.Add(Rig); });

	const AActor* ViewTarget = CameraManager->GetViewTarget();
	const AActor* PendingTarget = CameraManager->PendingViewTarget.Target;

	for (UKzCameraRigComponent* Rig : Rigs)
	{
		const AActor* Owner = Rig->GetOwner();
		if (Owner == ViewTarget || Owner == PendingTarget)
		{
			Rig->Wake();
		}
		else if (Rig->IsAwake())
		{
			Rig->Sleep();
		}
	}
}

const AKzCameraZone* UKzCameraDirector::FindZone(const FVector& Location) const
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return nullptr;

	const AKzCameraZone* Best = nullptr;
	double BestSize = 0.0;

	Registry->ForEach<AKzCameraZone>([&](const AKzCameraZone* Zone)
	{
		if (!Zone->Contains(Location)) return;

		const double Size = Zone->GetSize();
		if (!Best || Zone->Priority > Best->Priority || (Zone->Priority == Best->Priority && Size < BestSize))
		{
			Best = Zone;
			BestSize = Size;
		}
	});

	return Best;
}

AActor* UKzCameraDirector::FindNearestCamera(const FVector& Location) const
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return nullptr;

	AActor* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();

	Registry->ForEach<UKzCameraRigComponent>([&](const UKzCameraRigComponent* Rig)
	{
		const double Distance = Rig->GetDistanceTo(Location);
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Rig->GetOwner();
		}
	});

	return Nearest;
}