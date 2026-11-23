// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Math/Interval.h"
#include "KzTimeStop.generated.h"

class AActor;

/** Time stopped by a hit, for attacks to share: how long, how it lets go, when it starts, and whether each impact sets it off again. */
UCLASS(Abstract, BlueprintType)
class KZGAMEPLAY_API UKzTimeStop : public UDataAsset
{
	GENERATED_BODY()

public:
	/** How long it lasts once started. An attack can set its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (ClampMin = "0", Units = "Seconds"))
	float Duration = 0.1f;

	/**
	 * The speed along the way: X from 0 to 1 through Duration, Y from 0, the slowest, to 1, normal speed. A Hit Stop's slowest is frozen; a Slow
	 * Motion's, its Time Dilation's Min. Without keys, the slowest to the end.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop")
	FRuntimeFloatCurve Curve;

	/** How long after the hit it starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (ClampMin = "0", Units = "Seconds"))
	float Delay = 0.0f;

	/** Every impact of an attack sets it off. Off, only the attack's first impact does, with all it hits in that instant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop")
	bool bRepeatInAttack = true;
};

/** Who a Hit Stop holds. */
UENUM(BlueprintType)
enum class EKzHitStopScope : uint8
{
	InstigatorAndReceiver UMETA(DisplayName = "Instigator And Receiver"),

	/** Only who takes the hit, such as from a projectile. */
	Receiver,

	/** Only who dealt it, such as a parried attacker. */
	Instigator
};

/** Freezes or slows those on either side of a hit, while the rest of the world goes on. */
UCLASS(BlueprintType, meta = (DisplayName = "Hit Stop"))
class KZGAMEPLAY_API UKzHitStop : public UKzTimeStop
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Stop")
	EKzHitStopScope Scope = EKzHitStopScope::InstigatorAndReceiver;

	/**
	 * Abilities with any of these among their Asset Tags let go of whoever activates them, such as Ability.Kind.Defense for a dodge out of it; the other
	 * side stays held. Empty, nothing does.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Stop")
	FGameplayTagContainer CancelableBy;

	/** Holds Instigator and Receivers, as Scope says, for InDuration. */
	void Apply(AActor* Instigator, TConstArrayView<AActor*> Receivers, float InDuration) const;
};

/** Slows the whole game down, the camera aside. */
UCLASS(BlueprintType, meta = (DisplayName = "Slow Motion"))
class KZGAMEPLAY_API UKzSlowMotion : public UKzTimeStop
{
	GENERATED_BODY()

public:
	UKzSlowMotion();

	/** The game's speed where the curve is 0, Min, and where it is 1, Max. The curve stays within them even where it goes past 0 or 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slow Motion")
	FFloatInterval TimeDilation = FFloatInterval(0.2f, 1.0f);

	/** Slows WorldContext's world for InDuration of real time. */
	void Apply(const UObject* WorldContext, float InDuration) const;
};

/** The Hit Stop and the Slow Motion a hit sets off, each with its own duration if it overrides the asset's. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzTimeStops
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (DisplayThumbnail = "false"))
	TObjectPtr<const UKzHitStop> HitStop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (InlineEditConditionToggle))
	bool bOverrideHitStopDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (EditCondition = "bOverrideHitStopDuration", ClampMin = "0", Units = "Seconds"))
	float HitStopDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (DisplayThumbnail = "false"))
	TObjectPtr<const UKzSlowMotion> SlowMotion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (InlineEditConditionToggle))
	bool bOverrideSlowMotionDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Stop", meta = (EditCondition = "bOverrideSlowMotionDuration", ClampMin = "0", Units = "Seconds"))
	float SlowMotionDuration = 0.5f;

	float GetHitStopDuration() const { return bOverrideHitStopDuration ? HitStopDuration : HitStop ? HitStop->Duration : 0.0f; }
	float GetSlowMotionDuration() const { return bOverrideSlowMotionDuration ? SlowMotionDuration : SlowMotion ? SlowMotion->Duration : 0.0f; }
};