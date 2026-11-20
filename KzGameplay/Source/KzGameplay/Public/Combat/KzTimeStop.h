// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
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
	 * How strongly it holds along the way: X from 0 to 1 through Duration, Y from 1, frozen, to 0, normal speed, and slowed in between. Without keys,
	 * frozen to the end.
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

	/** Slows WorldContext's world for InDuration of real time. */
	void Apply(const UObject* WorldContext, float InDuration) const;
};

/** A Hit Stop an attack sets off, with its own duration if it overrides the asset's. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzHitStopSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Stop")
	TObjectPtr<const UKzHitStop> HitStop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Stop", meta = (InlineEditConditionToggle))
	bool bOverrideDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Stop", meta = (EditCondition = "bOverrideDuration", ClampMin = "0", Units = "Seconds"))
	float Duration = 0.1f;

	float GetDuration() const { return bOverrideDuration ? Duration : HitStop ? HitStop->Duration : 0.0f; }
};

/** A Slow Motion an attack sets off, with its own duration if it overrides the asset's. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzSlowMotionSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slow Motion")
	TObjectPtr<const UKzSlowMotion> SlowMotion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slow Motion", meta = (InlineEditConditionToggle))
	bool bOverrideDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slow Motion", meta = (EditCondition = "bOverrideDuration", ClampMin = "0", Units = "Seconds"))
	float Duration = 0.5f;

	float GetDuration() const { return bOverrideDuration ? Duration : SlowMotion ? SlowMotion->Duration : 0.0f; }
};