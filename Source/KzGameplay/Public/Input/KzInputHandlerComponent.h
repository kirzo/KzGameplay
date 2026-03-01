// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "KzInputHandlerComponent.generated.h"

class UKzInputProfile;
class UEnhancedInputComponent;
class APawn;

/**
 * Component responsible for translating Enhanced Input Actions into Gameplay Tags
 * and injecting them into the Gameplay Ability System (GAS) or broadcasting them.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzInputHandlerComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	/** The default input profile to use. Can be overridden at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UKzInputProfile> DefaultInputProfile;

private:
	/** Stores the handles of current bindings so we can cleanly remove them on profile swaps. */
	TArray<uint32> BindHandles;

public:
	UKzInputHandlerComponent();

	/** Manually re-initializes input with a new profile (e.g., when swapping control schemes). */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void InitializeInput(UKzInputProfile* OverrideProfile);

protected:
	virtual void BeginPlay() override;

private:
	/** Called by the Pawn when it has been locally restarted and the InputComponent is ready. */
	UFUNCTION()
	void OnPawnRestarted(APawn* Pawn);

	/** Internal helper to perform the actual binding. */
	void TryBindInput(APawn* Pawn, UKzInputProfile* ProfileToUse = nullptr);

	/** Internal callback for when an input action is pressed. */
	void Input_ActionPressed(FGameplayTag InputTag);

	/** Internal callback for when an input action is released. */
	void Input_ActionReleased(FGameplayTag InputTag);
};