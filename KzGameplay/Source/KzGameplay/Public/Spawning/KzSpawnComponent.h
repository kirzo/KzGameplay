// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/TimerHandle.h"
#include "Engine/LatentActionManager.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "Sensors/KzSensorComponent.h"
#include "UObject/ObjectKey.h"
#include "KzSpawnComponent.generated.h"

class UScriptableActionRunner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzSpawnSignature, AActor*, Actor);

/**
 * Spawns actors where it is, such as enemies coming into a fight: new ones, or ones from the actor pool.
 *
 * It marks where the bottom of the actor's collision goes, so placed on the ground the actor stands on it. When something is in the way, or with
 * Require Nav Mesh the spot is off the navmesh, it looks around within Search Radius; when nothing fits it tries again every Retry Interval, and
 * out of retries it logs who, where and why to LogKzSpawn. kz.Spawn.Draw shows the spots it tries.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzSpawnComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UKzSpawnComponent();

	/** How far around the point it looks for room when the point has none. 0 tries only the point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0, Units = "Centimeters"))
	float SearchRadius = 200.0f;

	/** The spot must be on the navmesh, such as for whoever walks away from it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bRequireNavMesh = true;

	/** Seconds between tries while nothing fits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0.1, Units = "Seconds"))
	float RetryInterval = 1.0f;

	/** Tries after the first before it gives up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0))
	int32 MaxRetries = 5;

	/** Takes its actors from the actor pool, and Despawn gives them back, instead of spawning and destroying them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bUsePool = false;

	/** Runs on each actor it spawns, such as an entrance animation. Its context has the Actor and the Spawn Point, this component's owner. */
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (DisplayName = "Spawn Action"))
	FScriptableAction SpawnAction;

	/** Spawned as the level starts, such as an enemy already waiting there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Spawn")
	TSubclassOf<AActor> AutoSpawnClass;

	/** Spawns it instead once the owner's Sensor.Spawn sensor senses something, such as the player coming close. That sensor turns off then. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Spawn", meta = (EditCondition = "AutoSpawnClass != nullptr", EditConditionHides))
	bool bWaitForSensor = false;

	/** After each actor it spawns, with its Spawn Action already running. */
	UPROPERTY(BlueprintAssignable, Category = "Spawn")
	FKzSpawnSignature OnSpawned;

	/** Spawns an actor of Class here as soon as there is room. OnFinished gets it, or null once it gives up. */
	void RequestSpawn(TSubclassOf<AActor> Class, TFunction<void(AActor*)> OnFinished = nullptr);

	/** Takes away an actor it spawned, stopping its Spawn Action: back to the pool with Use Pool, destroyed without it. */
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void Despawn(AActor* Actor);

	/** The actors it spawned that it hasn't despawned yet. */
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void GetSpawnedActors(TArray<AActor*>& OutActors) const;

	/** Where an actor of Class would spawn now; false, with why, when nothing fits. */
	bool FindSpawnTransform(TSubclassOf<AActor> Class, FTransform& OutTransform, FString& OutWhy) const;

	/** The collision of Class that has to fit: its root's, if its root collides. */
	static const UPrimitiveComponent* GetSpawnCollision(TSubclassOf<AActor> Class);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FRequest
	{
		TSubclassOf<AActor> Class;
		TFunction<void(AActor*)> OnFinished;
		int32 Retries = 0;
	};

	TArray<FRequest> Pending;
	FTimerHandle RetryTimer;
	TWeakObjectPtr<UKzSensorComponent> WaitingSensor;
	TArray<TWeakObjectPtr<AActor>> Spawned;

	/** The Spawn Action running on each actor, to stop it on Despawn. */
	TMap<TObjectKey<AActor>, TWeakObjectPtr<UScriptableActionRunner>> RunningActions;

	/** Spawns the request's actor or gives up on it; false while it waits for another try. */
	bool Attempt(FRequest& Request);

	AActor* Spawn(TSubclassOf<AActor> Class, FString& OutWhy);

	void Retry();

	UFUNCTION()
	void HandleSensed(const FKzOverlapResult& OverlapInfo);
};

UENUM(BlueprintType)
enum class EKzSpawnResult : uint8
{
	Spawned,
	Failed
};

UCLASS()
class KZGAMEPLAY_API UKzSpawnLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Spawns an actor of Class at a Spawn Point, or any actor with a Spawn Component, as soon as it has room. Actor comes out as Class.
	 * Failed once the point gives up.
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawn", meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", DeterminesOutputType = "Class", DynamicOutputParam = "Actor"))
	static void SpawnAtPoint(AActor* SpawnPoint, TSubclassOf<AActor> Class, EKzSpawnResult& Result, AActor*& Actor, FLatentActionInfo LatentInfo);
};