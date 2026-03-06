// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "KzMeleeWeaponComponent.generated.h"

class UMeshComponent;

/**
 * Component responsible for handling melee weapon collision traces and hits.
 * Designed to be attached to the weapon actor itself.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzMeleeWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzMeleeWeaponComponent();

	/** Target mesh to use for the socket traces (e.g., the sword or axe mesh). */
	UPROPERTY(BlueprintReadWrite, Category = "Combat|Melee")
	TWeakObjectPtr<UMeshComponent> WeaponMesh;

	/** The object types this weapon should hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	TSet<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	/** If true, the trace will be drawn in the world for debugging purposes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	bool bDebugTrace = false;

	/** How long the debug trace lines will remain visible in the world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee", meta = (EditCondition = "bDebugTrace", EditConditionHides))
	float DebugDuration = 2.0f;

	/** Starts the trace window, resetting hit data and caching the initial position. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void StartMeleeTrace(FName BaseSocket, FName TipSocket, float Radius);

	/** Performs the volumetric sweep from the previous frame to the current frame. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void SweepMeleeTrace(FGameplayTag HitEventTag);

	/** Ends the trace window and performs any necessary cleanup. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Melee")
	void EndMeleeTrace();

protected:
	virtual void BeginPlay() override;

	/** Actors hit during the current attack sequence to prevent multi-hitting the same target. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitActors;

	FName ActiveBaseSocket;
	FName ActiveTipSocket;
	float ActiveTraceRadius;

	FVector PreviousCenter;
	bool bIsTracing;
};