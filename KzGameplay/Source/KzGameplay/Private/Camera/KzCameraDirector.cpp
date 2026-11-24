// Copyright 2026 kirzo

#include "Camera/KzCameraDirector.h"
#include "Camera/KzCameraRigComponent.h"
#include "Camera/KzCameraZone.h"
#include "Core/KzRegistrySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogKzCamera, Log, All);

namespace
{
	constexpr int32 MaxHistory = 12;
}

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

FString UKzCameraDirector::Describe(const UObject* Object)
{
	if (const UActorComponent* Component = Cast<UActorComponent>(Object))
	{
		Object = Component->GetOwner();
	}

	const AActor* Actor = Cast<AActor>(Object);
	return Actor ? Actor->GetActorNameOrLabel() : GetNameSafe(Object);
}

void UKzCameraDirector::RecordEvent(const UObject* Subject, const FString& Text)
{
	UE_VLOG(Subject, LogKzCamera, Log, TEXT("%s"), *Text);

	UKzCameraDirector* Director = Get(Subject);
	if (!Director) return;

	if (Director->History.Num() >= MaxHistory)
	{
		Director->History.RemoveAt(0);
	}
	Director->History.Add({ Director->GetWorld()->GetTimeSeconds(), Text });
}

void UKzCameraDirector::SwitchTo(AActor* Camera, const FViewTargetTransitionParams& Blend, const UObject* Instigator)
{
	Show(Camera, Blend, Instigator ? Describe(Instigator) : FString(TEXT("C++")));
}

void UKzCameraDirector::ReturnToGameplay(const FViewTargetTransitionParams& Blend, const UObject* Instigator)
{
	Show(GameplayCamera.Get(), Blend, FString::Printf(TEXT("back to gameplay, by %s"), Instigator ? *Describe(Instigator) : TEXT("C++")));
}

void UKzCameraDirector::Show(AActor* Camera, const FViewTargetTransitionParams& Blend, const FString& Reason)
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

	LastSwitch = { GetWorld()->GetTimeSeconds(), Reason };
	const FString How = Blend.BlendTime > 0.0f ? FString::Printf(TEXT("Blend %.1f s to"), Blend.BlendTime) : FString(TEXT("Cut to"));
	RecordEvent(this, FString::Printf(TEXT("%s %s (%s)"), *How, *Describe(Camera), *Reason));
}

void UKzCameraDirector::SelectCamera()
{
	const APlayerController* PlayerController = GetPlayerController();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn) return;

	const FVector Location = Pawn->GetActorLocation();
	const AKzCameraZone* Zone = FindZone(Location);
	CurrentZone = Zone;

	if (Zone && Zone->Camera)
	{
		Show(Zone->Camera, FViewTargetTransitionParams(), FString::Printf(TEXT("the player is in %s"), *Describe(Zone)));
	}
	else
	{
		Show(FindNearestCamera(Location), FViewTargetTransitionParams(), TEXT("nearest to the player, in no zone"));
	}
}

void UKzCameraDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PlayerController = GetPlayerController();
	const APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager.Get() : nullptr;
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!CameraManager || !Pawn) return;

	const FVector Location = Pawn->GetActorLocation();
	const bool bNewPawn = Pawn != LastPawn.Get();
	const bool bJumped = bNewPawn || (TeleportDistance > 0.0f && FVector::DistSquared(Location, LastPawnLocation) > FMath::Square(TeleportDistance));

	if (bJumped)
	{
		// Play start, a respawn or a teleport: cut to the camera for where the player is now.
		RecordEvent(this, bNewPawn
			? FString::Printf(TEXT("Player pawn is %s"), *Describe(Pawn))
			: FString::Printf(TEXT("Player jumped %.0f cm"), FVector::Dist(Location, LastPawnLocation)));
		LastPawn = Pawn;
		LastPawnLocation = Location;
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
				SwitchTo(Zone->Camera, Zone->Blend, Zone);
			}
			else
			{
				// A cutscene or a fixed shot is in view: returning to gameplay will go to the zone's camera.
				GameplayCamera = Zone->Camera.Get();
				RecordEvent(this, FString::Printf(TEXT("Entered %s during another shot: back to gameplay goes to %s"), *Describe(Zone), *Describe(Zone->Camera)));
			}
		}
		CurrentZone = Zone;
		LastPawnLocation = Location;
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