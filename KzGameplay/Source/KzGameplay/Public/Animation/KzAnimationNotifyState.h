// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "KzAnimationNotifyState.generated.h"

UENUM()
enum class EKzAnimTimeUnit : uint8
{
	Seconds,

	/** Frames of the animation it is in, at its frame rate. */
	Frames
};

/** A stretch of an animation's time, set in seconds or in its frames. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzAnimTime
{
	GENERATED_BODY()

	FKzAnimTime() = default;
	explicit FKzAnimTime(float InValue, EKzAnimTimeUnit InUnit = EKzAnimTimeUnit::Seconds) : Value(InValue), Unit(InUnit) {}

	UPROPERTY(EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Time")
	EKzAnimTimeUnit Unit = EKzAnimTimeUnit::Seconds;

	/** In seconds. Frames count at the animation's frame rate, or 30 a second without one. */
	float ToSeconds(const UAnimSequenceBase* Animation) const;
};

/** Notify state handed to the animation source play that fired it, which tells whoever started that play. Nothing hears it without one, as in the animation editors. */
UCLASS(Abstract)
class KZGAMEPLAY_API UKzAnimationNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};