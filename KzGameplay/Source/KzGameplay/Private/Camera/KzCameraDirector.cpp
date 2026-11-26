// Copyright 2026 kirzo

#include "Camera/KzCameraDirector.h"
#include "Actors/KzStandIn.h"
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
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
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

const AActor* UKzCameraDirector::GetPlayer() const
{
	if (GetWorld()->IsGameWorld())
	{
		const APlayerController* PlayerController = GetPlayerController();
		return PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	// The selected stand-in, or else the first.
	const AKzStandIn* Found = nullptr;
#if WITH_EDITOR
	if (UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this))
	{
		Registry->ForEach<AKzStandIn>([&Found](const AKzStandIn* StandIn)
		{
			if (!Found || StandIn->IsSelectedInEditor())
			{
				Found = StandIn;
			}
		});
	}
#endif
	return Found;
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

void UKzCameraDirector::Show(AActor* Camera, const FViewTargetTransitionParams& Blend, const FString& Reason, bool bPlayerJumped)
{
	// In the editor there is no view to set: the chosen camera feeds the stand-in's preview.
	APlayerController* PlayerController = GetPlayerController();
	if (!Camera || (!PlayerController && GetWorld()->IsGameWorld())) return;

	// Awake and in place before the blend reads its view.
	if (UKzCameraRigComponent* Rig = Camera->FindComponentByClass<UKzCameraRigComponent>())
	{
		GameplayCamera = Camera;
		Rig->Wake(bPlayerJumped);
	}

	if (PlayerController)
	{
		PlayerController->SetViewTarget(Camera, Blend);
	}

	LastSwitch = { GetWorld()->GetTimeSeconds(), Reason };
	const FString How = Blend.BlendTime > 0.0f ? FString::Printf(TEXT("Blend %.1f s to"), Blend.BlendTime) : FString(TEXT("Cut to"));
	RecordEvent(this, FString::Printf(TEXT("%s %s (%s)"), *How, *Describe(Camera), *Reason));
}

void UKzCameraDirector::SelectCamera()
{
	const AActor* Player = GetPlayer();
	if (!Player) return;

	const FVector Location = Player->GetTargetLocation();
	const AKzCameraZone* Zone = FindZone(this, Location);
	CurrentZone = Zone;

	if (Zone && Zone->Camera)
	{
		Show(Zone->Camera, FViewTargetTransitionParams(), FString::Printf(TEXT("the player is in %s"), *Describe(Zone)), true);
	}
	else
	{
		Show(FindNearestCamera(this, Location), FViewTargetTransitionParams(), TEXT("nearest to the player, in no zone"), true);
	}
}

void UKzCameraDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// In the editor there is no camera manager: the stand-in plays the player, and nothing but gameplay is ever shown.
	const bool bGame = GetWorld()->IsGameWorld();
	APlayerController* PlayerController = GetPlayerController();
	const APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager.Get() : nullptr;
	const AActor* Player = GetPlayer();
	if (!bGame && !Player)
	{
		// The stand-in is gone: every camera previews on its own again.
		LastPlayer.Reset();
		UpdateEditorRigs(nullptr);
		return;
	}
	if (!Player || (bGame && !CameraManager)) return;

	const FVector Location = Player->GetTargetLocation();
	const bool bNewPlayer = Player != LastPlayer.Get();
	const bool bJumped = bNewPlayer || (TeleportDistance > 0.0f && FVector::DistSquared(Location, LastPlayerLocation) > FMath::Square(TeleportDistance));

	if (bJumped)
	{
		// Play start, a respawn or a teleport: cut to the camera for where the player is now.
		RecordEvent(this, bNewPlayer
			? FString::Printf(TEXT("Player is %s"), *Describe(Player))
			: FString::Printf(TEXT("Player jumped %.0f cm"), FVector::Dist(Location, LastPlayerLocation)));
		LastPlayer = Player;
		LastPlayerLocation = Location;
		SelectCamera();
	}
	else
	{
		// Entering a zone switches to its camera; leaving every zone keeps the current one.
		const AKzCameraZone* Zone = FindZone(this, Location);
		if (Zone && Zone != CurrentZone.Get() && Zone->Camera && Zone->Camera != GameplayCamera.Get())
		{
			if (!CameraManager || IsShowingGameplay(CameraManager))
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
		LastPlayerLocation = Location;
	}

	if (CameraManager)
	{
		UpdateRigs(CameraManager);
	}
	else
	{
		UpdateEditorRigs(Player);
	}
}

void UKzCameraDirector::UpdateEditorRigs(const AActor* Player) const
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(this);
	if (!Registry) return;

	TArray<UKzCameraRigComponent*, TInlineAllocator<8>> Rigs;
	Registry->ForEach<UKzCameraRigComponent>([&Rigs](UKzCameraRigComponent* Rig) { Rigs.Add(Rig); });

	for (UKzCameraRigComponent* Rig : Rigs)
	{
		if (!Player)
		{
			Rig->Wake();
			continue;
		}
		if (Rig->GetTarget() != Player) continue;

		if (Rig->GetOwner() == GameplayCamera.Get())
		{
			Rig->Wake();
		}
		else if (Rig->IsAwake())
		{
			Rig->Sleep();
		}
	}
}

bool UKzCameraDirector::IsShowing(const AActor* Camera) const
{
	const APlayerController* PlayerController = GetPlayerController();
	const APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager.Get() : nullptr;
	return Camera && CameraManager && (CameraManager->GetViewTarget() == Camera || CameraManager->PendingViewTarget.Target == Camera);
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

const AKzCameraZone* UKzCameraDirector::FindZone(const UObject* WorldContextObject, const FVector& Location)
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(WorldContextObject);
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

AActor* UKzCameraDirector::FindNearestCamera(const UObject* WorldContextObject, const FVector& Location)
{
	UKzRegistrySubsystem* Registry = UKzRegistrySubsystem::Get(WorldContextObject);
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