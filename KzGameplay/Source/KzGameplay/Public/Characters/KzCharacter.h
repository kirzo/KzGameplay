// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Containers/KzPriorityStack.h"
#include "Curves/CurveFloat.h"
#include "KzCharacter.generated.h"

class UKzAbilitySystemComponent;
class UKzCapabilityComponent;
class UKzInputHandlerComponent;
struct FInputActionValue;
enum class ETriggerEvent : uint8;

/** How a walk speed source sets the max walk speed. */
UENUM(BlueprintType)
enum class EKzWalkSpeedMode : uint8
{
	/** Speed, as it is. */
	Absolute,

	/** The base walk speed times Multiplier. */
	Multiplier
};

USTRUCT(BlueprintType)
struct FKzWalkSpeed
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EKzWalkSpeedMode Mode = EKzWalkSpeedMode::Absolute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0, EditCondition = "Mode == EKzWalkSpeedMode::Absolute", EditConditionHides))
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0, EditCondition = "Mode == EKzWalkSpeedMode::Multiplier", EditConditionHides))
	float Multiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	int32 Priority = 0;

	/** The max walk speed it sets over a base walk speed. */
	float Resolve(float BaseSpeed) const { return Mode == EKzWalkSpeedMode::Multiplier ? BaseSpeed * Multiplier : Speed; }
};

/**
 * Base character with an ability system, capabilities, input and walk speed by source.
 *
 * Every character has an input handler so any of them can be possessed: without a player controller it binds
 * nothing, and the profiles its capabilities push are bound when a player takes it.
 */
UCLASS(Abstract)
class KZGAMEPLAY_API AKzCharacter : public ACharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AKzCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	/** The highest-priority source sets the max walk speed; on a tie, the last one pushed. Pushing a source again replaces it. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void PushMaxWalkSpeed(FName SourceID, const FKzWalkSpeed& WalkSpeed);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void RemoveMaxWalkSpeed(FName SourceID);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UKzAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UKzCapabilityComponent> CapabilityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UKzInputHandlerComponent> InputHandlerComponent;

	/** Walk speeds pushed while the character has the tag, with the tag's name as the source. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	TMap<FGameplayTag, FKzWalkSpeed> WalkSpeedByTag;

	/**
	 * Multiplies the max walk speed, whatever source sets it, by the slope of the ground along the movement, in degrees: positive uphill,
	 * negative downhill. Without keys, slopes change nothing.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FRuntimeFloatCurve SlopeSpeedCurve;

	/** How fast the slope multiplier follows the ground, so a bump does not jolt the speed. 0 follows it at once. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = 0))
	float SlopeInterpSpeed = 5.0f;

	/**
	 * Yaw rotation rate by ground speed, in degrees per second: fast from a standstill, so it does not slide sideways while it turns, and slower
	 * at a run, so it arcs. Without keys, the movement component's Rotation Rate stays.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FRuntimeFloatCurve RotationRateBySpeed;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

	/** Move goes to the movement component and Look turns the control rotation. Override for other schemes, such as twin-stick aiming. */
	UFUNCTION()
	virtual void HandleInputAxis(FGameplayTag InputTag, const FInputActionValue& Value, ETriggerEvent TriggerEvent);

private:
	/** Max walk speed with no source pushed: the movement component's at BeginPlay. */
	float BaseWalkSpeed = 0.0f;

	Kz::TPriorityStack<FKzWalkSpeed, false, FName, false> WalkSpeedStack;

	float SlopeMultiplier = 1.0f;

	void UpdateMaxWalkSpeed();

	void UpdateSlopeMultiplier(float DeltaSeconds);

	void UpdateRotationRate();

	void HandleWalkSpeedTagChanged(const FGameplayTag Tag, int32 NewCount);
};
