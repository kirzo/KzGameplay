// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzShapeComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "ScriptableConditions/ScriptableRequirement.h"
#include "KzSensorComponent.generated.h"

class UKzSensableComponent;

/** Struct to pair the logical Actor with the specific sensable component that triggered detection */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzOverlapResult
{
	GENERATED_BODY()

	/** The high-level Actor owning the SensableComponent */
	UPROPERTY(BlueprintReadOnly, Category = "KzSensor")
	TWeakObjectPtr<AActor> Actor;

	/** The specific component that caused the overlap */
	UPROPERTY(BlueprintReadOnly, Category = "KzSensor")
	TWeakObjectPtr<UKzSensableComponent> SensableComponent;

	bool operator==(const AActor* Other) const { return Actor.Get() == Other; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzSensorObjectEvent, const FKzOverlapResult&, OverlapInfo);

/**
 * A sensor component that detects UKzSensableComponents.
 * Evaluates Gameplay Tags natively and applies Scriptable Requirements for logical filtering.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzSensorComponent : public UKzShapeComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	UKzSensorComponent();

	// --- Configuration ---

	/** What the sensor is for, so whoever needs one finds it among the actor's sensors, such as Sensor.Targeting for the combo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KzSensor")
	FGameplayTagContainer GameplayTags;

	/** How often to perform the geometric scan (in seconds). 0 never scans on its own: it only answers Query. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KzSensor", meta = (ClampMin = "0", Units = "Seconds"))
	float ScanInterval = 0.1f;

	/** Fast native filter: Only objects matching this query will be evaluated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KzSensor")
	FGameplayTagQuery SearchTagQuery;

	/** Logical filter: Complex rules (e.g., Line of Sight, IsAlive) that candidates must pass. */
	UPROPERTY(EditAnywhere, Category = "KzSensor")
	FScriptableRequirement DetectionRequirement;

	/** The last actor detected by the sensor. */
	UPROPERTY(VisibleAnywhere, Category = "KzSensor")
	TObjectPtr<AActor> LastDetectedActor = nullptr;

	// --- Events ---

	/** Fired when an object enters the sensor and passes all filters. */
	UPROPERTY(BlueprintAssignable, Category = "KzSensor|Events")
	FKzSensorObjectEvent OnObjectBeginOverlap;

	/** Fired when an object exits the sensor OR fails the filters (e.g., stealth activated). */
	UPROPERTY(BlueprintAssignable, Category = "KzSensor|Events")
	FKzSensorObjectEvent OnObjectEndOverlap;

	// --- Public API ---

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer.AppendTags(GameplayTags); }

	/** What a shape at a transform senses right now, through this sensor's filters, without touching what it keeps or firing its events. */
	void Query(const FKzShapeInstance& QueryShape, const FTransform& QueryTransform, TArray<FKzOverlapResult>& OutResults);

	/** Returns the full overlap info (Actor + Sensable Component). */
	UFUNCTION(BlueprintPure, Category = "KzSensor")
	const TArray<FKzOverlapResult>& GetOverlapInfos() const { return CachedOverlaps; }

	/** Returns all currently sensed actors matching the specified class. */
	UFUNCTION(BlueprintCallable, Category = "KzSensor", meta = (DeterminesOutputType = "ActorClass", DynamicOutputParam = "OutActors"))
	void GetSensedActors(TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors) const;

	/** Returns the list of currently sensed actors cast to T. */
	template <typename T>
	TArray<T*> GetSensedActors() const
	{
		TArray<T*> Result;
		Result.Reserve(CachedOverlaps.Num());

		for (const FKzOverlapResult& Overlap : CachedOverlaps)
		{
			if (T* CastedActor = Cast<T>(Overlap.Actor.Get()))
			{
				Result.Add(CastedActor);
			}
		}
		return Result;
	}

	/** Scans only while active: deactivated, it stops and loses what it sensed, firing its end events. */
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void StartScanning();

	/** The timer function that periodically scans the world. */
	void PerformScan();

private:
	/** The cache of currently detected logical objects. */
	UPROPERTY(Transient)
	TArray<FKzOverlapResult> CachedOverlaps;

	/** Per-scan scratch, swapped with CachedOverlaps each scan to avoid copies. */
	UPROPERTY(Transient)
	TArray<FKzOverlapResult> ScratchOverlaps;

	/** Per-scan scratch for the spatial query candidates. */
	TArray<UKzSensableComponent*> ScratchSensables;

	FTimerHandle ScanTimerHandle;
};