// Copyright 2026 kirzo

#include "Spawning/KzSpawnComponent.h"
#include "KzDrawDebugHelpers.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Pooling/KzActorPoolSubsystem.h"
#include "ScriptableContext.h"
#include "ScriptableTasks/ScriptableActionRunner.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKzSpawn, Log, All);

namespace
{
	TAutoConsoleVariable<bool> CVarDrawSpawn(TEXT("kz.Spawn.Draw"), false, TEXT("Draws the spots spawn points try: green where the actor spawns, red where something is in the way, yellow off the navmesh."));

	/** Shrinks the collision tested, so touching the ground isn't something in the way. */
	constexpr float GroundContact = 5.0f;
}

UKzSpawnComponent::UKzSpawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SpawnAction.AddContextProperty<AActor*>(TEXT("Actor"));
	SpawnAction.AddContextProperty<AActor*>(TEXT("SpawnPoint"));
}

void UKzSpawnComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!AutoSpawnClass)
	{
		return;
	}

	if (AutoSpawnDistance <= 0.0f)
	{
		RequestSpawn(AutoSpawnClass);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(AutoSpawnTimer, this, &ThisClass::CheckAutoSpawn, 0.25f, true);
	}
}

void UKzSpawnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(RetryTimer);
	GetWorld()->GetTimerManager().ClearTimer(AutoSpawnTimer);

	for (FRequest& Request : TArray<FRequest>(MoveTemp(Pending)))
	{
		if (Request.OnFinished)
		{
			Request.OnFinished(nullptr);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UKzSpawnComponent::CheckAutoSpawn()
{
	const float MaxDistanceSquared = FMath::Square(AutoSpawnDistance);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Player = It->Get();
		const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (Pawn && FVector::DistSquared(Pawn->GetActorLocation(), GetComponentLocation()) <= MaxDistanceSquared)
		{
			GetWorld()->GetTimerManager().ClearTimer(AutoSpawnTimer);
			RequestSpawn(AutoSpawnClass);
			return;
		}
	}
}

void UKzSpawnComponent::RequestSpawn(TSubclassOf<AActor> Class, TFunction<void(AActor*)> OnFinished)
{
	FRequest Request{ Class, MoveTemp(OnFinished) };
	if (Attempt(Request))
	{
		return;
	}

	Pending.Add(MoveTemp(Request));

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(RetryTimer))
	{
		TimerManager.SetTimer(RetryTimer, this, &ThisClass::Retry, RetryInterval, true);
	}
}

void UKzSpawnComponent::Retry()
{
	// What a spawn calls out to may request more: those go to Pending meanwhile
	for (FRequest& Request : TArray<FRequest>(MoveTemp(Pending)))
	{
		if (!Attempt(Request))
		{
			Pending.Add(MoveTemp(Request));
		}
	}

	if (Pending.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(RetryTimer);
	}
}

bool UKzSpawnComponent::Attempt(FRequest& Request)
{
	FString Why;
	AActor* Actor = Spawn(Request.Class, Why);

	if (!Actor && Request.Class && Request.Retries++ < MaxRetries)
	{
		return false;
	}

	if (!Actor)
	{
		UE_LOG(LogKzSpawn, Error, TEXT("%s couldn't spawn %s at %s after %d tries: %s."), *GetNameSafe(GetOwner()), *GetNameSafe(Request.Class.Get()), *GetComponentLocation().ToCompactString(), Request.Retries, *Why);
	}

	if (Request.OnFinished)
	{
		Request.OnFinished(Actor);
	}
	return true;
}

AActor* UKzSpawnComponent::Spawn(TSubclassOf<AActor> Class, FString& OutWhy)
{
	FTransform Transform;
	if (!FindSpawnTransform(Class, Transform, OutWhy))
	{
		return nullptr;
	}

	UKzActorPoolSubsystem* Pool = bUsePool ? UKzActorPoolSubsystem::Get(this) : nullptr;
	AActor* Actor = Pool ? Pool->Acquire(Class, Transform) : GetWorld()->SpawnActor<AActor>(Class, Transform);
	if (!Actor)
	{
		OutWhy = TEXT("the spawn itself failed");
		return nullptr;
	}

	if (!SpawnAction.Tasks.IsEmpty())
	{
		for (auto It = RunningActions.CreateIterator(); It; ++It)
		{
			if (!It.Key().ResolveObjectPtr() || !It.Value().IsValid())
			{
				It.RemoveCurrent();
			}
		}

		FScriptableContext Context;
		Context.SetProperty(TEXT("Actor"), Actor);
		Context.SetProperty(TEXT("SpawnPoint"), GetOwner());
		RunningActions.Add(TObjectKey<AActor>(Actor), FScriptableAction::RunCopy(this, SpawnAction, Context));
	}

	OnSpawned.Broadcast(Actor);
	return Actor;
}

void UKzSpawnComponent::Despawn(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	TWeakObjectPtr<UScriptableActionRunner> Runner;
	if (RunningActions.RemoveAndCopyValue(TObjectKey<AActor>(Actor), Runner) && Runner.IsValid())
	{
		Runner->Cancel();
	}

	UKzActorPoolSubsystem* Pool = bUsePool ? UKzActorPoolSubsystem::Get(this) : nullptr;
	if (Pool)
	{
		Pool->Release(Actor);
	}
	else
	{
		Actor->Destroy();
	}
}

const UPrimitiveComponent* UKzSpawnComponent::GetSpawnCollision(TSubclassOf<AActor> Class)
{
	const AActor* Default = Class ? Class->GetDefaultObject<AActor>() : nullptr;
	const UPrimitiveComponent* Root = Default ? Cast<UPrimitiveComponent>(Default->GetRootComponent()) : nullptr;
	return Root && Root->IsCollisionEnabled() ? Root : nullptr;
}

bool UKzSpawnComponent::FindSpawnTransform(TSubclassOf<AActor> Class, FTransform& OutTransform, FString& OutWhy) const
{
	OutWhy.Reset();

	UWorld* World = GetWorld();
	if (!World || !Class)
	{
		OutWhy = TEXT("it has no class to spawn");
		return false;
	}

	UNavigationSystemV1* NavSystem = bRequireNavMesh ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (bRequireNavMesh && !NavSystem)
	{
		OutWhy = TEXT("the level has no navigation");
		return false;
	}

	const UPrimitiveComponent* Collision = GetSpawnCollision(Class);
	const FVector Extent = Collision ? Collision->GetCollisionShape().GetExtent() : FVector::ZeroVector;
	const FCollisionShape Shape = Collision ? Collision->GetCollisionShape(-GroundContact) : FCollisionShape();

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KzSpawn), false);
	FCollisionResponseParams ResponseParams;
	if (Collision)
	{
		ResponseParams.CollisionResponse = Collision->GetCollisionResponseToChannels();
	}

	// The point, then rings around it a body apart out to Search Radius
	const FVector Point = GetComponentLocation();
	const FQuat Rotation = GetComponentQuat();
	const float Step = FMath::Max(static_cast<float>(Extent.X) * 2.0f, 50.0f);
	const int32 Rings = SearchRadius > 0.0f ? FMath::CeilToInt(SearchRadius / Step) : 0;

	TArray<FVector, TInlineAllocator<64>> Spots;
	Spots.Add(Point);
	for (int32 Ring = 1; Ring <= Rings; ++Ring)
	{
		const float Radius = SearchRadius * Ring / Rings;
		const int32 Count = FMath::Max(6, FMath::CeilToInt(UE_TWO_PI * Radius / Step));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = UE_TWO_PI * Index / Count;
			Spots.Add(Point + Rotation.RotateVector(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f)) * Radius);
		}
	}

	const bool bDraw = CVarDrawSpawn.GetValueOnGameThread();
	const FVector NavExtent(Step * 0.5f, Step * 0.5f, FMath::Max(Extent.Z, 50.0));
	TArray<FOverlapResult> Overlaps;

	for (const FVector& Spot : Spots)
	{
		FVector Ground = Spot;
		if (NavSystem)
		{
			FNavLocation NavLocation;
			if (!NavSystem->ProjectPointToNavigation(Spot, NavLocation, NavExtent))
			{
				if (bDraw)
				{
					DrawDebugPoint(World, Spot, 10.0f, FColor::Yellow, false, 3.0f);
				}
				if (OutWhy.IsEmpty())
				{
					OutWhy = TEXT("it is off the navmesh");
				}
				continue;
			}
			Ground = NavLocation.Location;
		}

		const FVector Location = Ground + FVector(0.0f, 0.0f, Extent.Z);

		Overlaps.Reset();
		if (Collision && World->OverlapMultiByChannel(Overlaps, Location, Rotation, Collision->GetCollisionObjectType(), Shape, QueryParams, ResponseParams))
		{
			if (bDraw)
			{
				DrawDebugShape(World, Location, Rotation, FKzShapeInstance::MakeFromCollisionShape(Shape), FColor::Red, false, 3.0f);
			}
			if (OutWhy.IsEmpty())
			{
				const FOverlapResult* Blocking = Overlaps.FindByPredicate([](const FOverlapResult& Overlap) { return Overlap.bBlockingHit; });
				OutWhy = FString::Printf(TEXT("%s is in the way"), Blocking ? *GetNameSafe(Blocking->GetActor()) : TEXT("something"));
			}
			continue;
		}

		if (bDraw && Collision)
		{
			DrawDebugShape(World, Location, Rotation, FKzShapeInstance::MakeFromCollisionShape(Shape), FColor::Green, false, 3.0f);
		}

		OutTransform = FTransform(Rotation, Location);
		return true;
	}

	if (Rings > 0)
	{
		OutWhy += FString::Printf(TEXT(", and nothing within %.0f cm fits"), SearchRadius);
	}
	return false;
}

UKzSpawnAtPoint* UKzSpawnAtPoint::SpawnAtPoint(AActor* SpawnPoint, TSubclassOf<AActor> Class)
{
	UKzSpawnAtPoint* Node = NewObject<UKzSpawnAtPoint>();
	Node->SpawnPoint = SpawnPoint;
	Node->Class = Class;
	Node->RegisterWithGameInstance(SpawnPoint);
	return Node;
}

void UKzSpawnAtPoint::Activate()
{
	UKzSpawnComponent* Component = SpawnPoint.IsValid() ? SpawnPoint->FindComponentByClass<UKzSpawnComponent>() : nullptr;
	if (!Component)
	{
		UE_LOG(LogKzSpawn, Error, TEXT("Spawn At Point: %s has no Spawn Component."), *GetNameSafe(SpawnPoint.Get()));
		Failed.Broadcast(nullptr);
		SetReadyToDestroy();
		return;
	}

	Component->RequestSpawn(Class, [WeakThis = TWeakObjectPtr<UKzSpawnAtPoint>(this)](AActor* Actor)
	{
		if (UKzSpawnAtPoint* This = WeakThis.Get())
		{
			(Actor ? This->Spawned : This->Failed).Broadcast(Actor);
			This->SetReadyToDestroy();
		}
	});
}