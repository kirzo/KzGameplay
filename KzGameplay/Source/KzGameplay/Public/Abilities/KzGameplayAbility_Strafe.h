// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Abilities/KzGameplayAbility.h"
#include "KzGameplayAbility_Strafe.generated.h"

class UKzInputHandlerComponent;
struct FInputActionValue;
enum class ETriggerEvent : uint8;

/**
 * Strafes while Input.Strafe is held: State.Strafing turns the character to its control rotation instead of its movement, through
 * AKzCharacter's Face Control Rotation Tags. An analog input, such as a stick, also points the controller along its world direction,
 * which is twin-stick facing; Camera Relative Direction among the input handler's modifiers makes a player's stick that direction.
 * A button leaves the controller where it looks: the camera, or an AI's focus. Granted by Capability.Strafe.
 */
UCLASS()
class KZGAMEPLAY_API UKzGameplayAbility_Strafe : public UKzGameplayAbility
{
	GENERATED_BODY()

public:
	UKzGameplayAbility_Strafe();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** The handler heard while active, so the binding comes off the one it went on. */
	TWeakObjectPtr<UKzInputHandlerComponent> InputHandler;

	/** Points the controller along a direction from one of this ability's inputs. */
	UFUNCTION()
	void HandleInputAxis(FGameplayTag AxisTag, const FInputActionValue& Value, ETriggerEvent TriggerEvent);
};
