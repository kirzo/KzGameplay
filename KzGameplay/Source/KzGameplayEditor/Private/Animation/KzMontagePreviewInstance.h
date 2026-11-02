// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "KzMontagePreviewInstance.generated.h"

class UAnimMontage;

USTRUCT()
struct FKzMontagePreviewProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FKzMontagePreviewProxy() = default;
	explicit FKzMontagePreviewProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	const UAnimMontage* Montage = nullptr;
	float Time = 0.0f;
	const UAnimMontage* FromMontage = nullptr;
	float FromTime = 0.0f;
	float FromWeight = 0.0f;
};

/**
 * Poses a montage at the time it is given, without an AnimBlueprint, and can show the one it takes over from underneath,
 * as the game does while a montage blends in. Posed rather than ticked, so a timeline lands on the same pose however it got there.
 */
UCLASS(Transient, NotBlueprintable)
class UKzMontagePreviewInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FKzMontagePreviewProxy;

public:
	/** Shows Montage from its start, playing. */
	void Play(UAnimMontage* InMontage);

	float GetTime() const { return Time; }
	void SetTime(float InTime) { Time = InTime; }
	bool IsPlaying() const { return bPlaying; }
	void SetPlaying(bool bInPlaying) { bPlaying = bInPlaying; }
	void SetPlayRate(float InPlayRate) { PlayRate = InPlayRate; }

	/** Shows InMontage at InTime under the montage, weighing it InWeight; zero shows the montage alone. */
	void SetBlendFrom(UAnimMontage* InMontage, float InTime, float InWeight);

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> FromMontage;

	float Time = 0.0f;
	float FromTime = 0.0f;
	float FromWeight = 0.0f;
	float PlayRate = 1.0f;
	bool bPlaying = false;
};
