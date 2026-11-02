// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Containers/KzPriorityStack.h"
#include "KzCharacter.generated.h"

class UKzAbilitySystemComponent;
class UKzCapabilityComponent;
class UKzInputHandlerComponent;
struct FInputActionValue;
enum class ETriggerEvent : uint8;

USTRUCT(BlueprintType)
struct FKzWalkSpeed
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = 0))
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	int32 Priority = 0;
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
	void PushMaxWalkSpeed(FName SourceID, float Speed, int32 Priority);

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

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	/** Move goes to the movement component and Look turns the control rotation. Override for other schemes, such as twin-stick aiming. */
	UFUNCTION()
	virtual void HandleInputAxis(FGameplayTag InputTag, const FInputActionValue& Value, ETriggerEvent TriggerEvent);

private:
	/** Max walk speed with no source pushed: the movement component's at BeginPlay. */
	float BaseWalkSpeed = 0.0f;

	Kz::TPriorityStack<float, false, FName, false> WalkSpeedStack;

	void UpdateMaxWalkSpeed();

	void HandleWalkSpeedTagChanged(const FGameplayTag Tag, int32 NewCount);
};
