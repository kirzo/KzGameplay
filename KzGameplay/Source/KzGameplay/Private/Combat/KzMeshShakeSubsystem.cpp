// Copyright 2026 kirzo

#include "Combat/KzMeshShakeSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/KzCombatSettings.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Below this much of a hit left on the screen, it goes into the screen and Screen Projected takes it across. */
	constexpr double MinOnScreen = 0.3;

	/** Direction turned as Kz Combat's Shake Space asks, by the first local player's camera; as it is without one. */
	FVector ToShakeSpace(const UWorld& World, const FVector& Direction)
	{
		const EKzShakeSpace Space = UKzCombatSettings::Get()->ShakeSpace;
		const APlayerController* Player = Space != EKzShakeSpace::Hit ? World.GetFirstPlayerController() : nullptr;
		const APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
		if (!Camera)
		{
			return Direction;
		}

		const FQuat View = Camera->GetCameraRotation().Quaternion();
		const FVector Right = View.GetRightVector();
		const FVector Across = FVector::DotProduct(Direction, Right) < 0.0 ? -Right : Right;
		if (Space == EKzShakeSpace::ScreenHorizontal)
		{
			return Across;
		}

		const FVector OnScreen = FVector::VectorPlaneProject(Direction, View.GetForwardVector());
		return OnScreen.Size() < MinOnScreen ? Across : OnScreen.GetSafeNormal();
	}
}

void UKzMeshShakeSubsystem::ShakeMesh(AActor* Actor, FVector Direction, float Amplitude, float Duration, float Frequency)
{
	UWorld* World = Actor ? Actor->GetWorld() : nullptr;
	UKzMeshShakeSubsystem* Subsystem = World ? World->GetSubsystem<UKzMeshShakeSubsystem>() : nullptr;
	const ACharacter* Character = Cast<ACharacter>(Actor);
	USceneComponent* Mesh = Character ? Character->GetMesh() : (Actor ? Actor->FindComponentByClass<UMeshComponent>() : nullptr);
	if (!Subsystem || !Mesh || Duration <= 0.0f || Amplitude <= 0.0f || Direction.IsNearlyZero())
	{
		return;
	}

	FShake* Shake = Subsystem->Shakes.Find(Mesh);
	if (!Shake)
	{
		Shake = &Subsystem->Shakes.Add(Mesh);
		Shake->BaseLocation = Mesh->GetRelativeLocation();
	}
	Shake->Direction = ToShakeSpace(*World, Direction.GetSafeNormal());
	Shake->Amplitude = Amplitude;
	Shake->Duration = Duration;
	Shake->Frequency = Frequency;
	Shake->Time = 0.0f;
}

void UKzMeshShakeSubsystem::Tick(float DeltaTime)
{
	for (auto It = Shakes.CreateIterator(); It; ++It)
	{
		USceneComponent* Mesh = It.Key().Get();
		if (!Mesh)
		{
			It.RemoveCurrent();
			continue;
		}

		FShake& Shake = It.Value();
		Shake.Time += DeltaTime;
		if (Shake.Time >= Shake.Duration)
		{
			Mesh->SetRelativeLocation(Shake.BaseLocation);
			It.RemoveCurrent();
			continue;
		}

		const float Offset = Shake.Amplitude * (1.0f - Shake.Time / Shake.Duration) * FMath::Cos(UE_TWO_PI * Shake.Frequency * Shake.Time);
		const FVector WorldOffset = Shake.Direction * Offset;
		const USceneComponent* Parent = Mesh->GetAttachParent();
		Mesh->SetRelativeLocation(Shake.BaseLocation + (Parent ? Parent->GetComponentTransform().InverseTransformVectorNoScale(WorldOffset) : WorldOffset));
	}
}

TStatId UKzMeshShakeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKzMeshShakeSubsystem, STATGROUP_Tickables);
}

void UKzMeshShakeSubsystem::Deinitialize()
{
	for (const TPair<TWeakObjectPtr<USceneComponent>, FShake>& Pair : Shakes)
	{
		if (USceneComponent* Mesh = Pair.Key.Get())
		{
			Mesh->SetRelativeLocation(Pair.Value.BaseLocation);
		}
	}
	Shakes.Reset();
	Super::Deinitialize();
}
