// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "KzAnimNotifyState_MeleeTrace.generated.h"

class UKzMeleeWeaponComponent;

/**
 * Animation notify state that triggers a volumetric sweep on the character's active melee weapon.
 * Relies on a UKzMeleeWeaponComponent being present on the weapon actor.
 */
UCLASS(meta = (DisplayName = "Melee Trace"))
class KZGAMEPLAY_API UKzAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_MeleeTrace();

	/** The equipment slot to query for the active melee weapon (e.g., Equipment.Slot.Hand.Right). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
	FGameplayTag EquipmentSlot;

	/** Tag sent to the Ability System when a hit is detected by the weapon component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
	FGameplayTag HitEventTag;

	/** Name of the socket at the base of the weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
	FName BaseSocketName;

	/** Name of the socket at the tip of the weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
	FName TipSocketName;

	/** Radius (thickness) of the capsule to trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trace")
	float TraceRadius = 15.0f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	/** Helper function to locate the active melee weapon component attached to the character. */
	UKzMeleeWeaponComponent* FindMeleeComponent(USkeletalMeshComponent* MeshComp) const;
};