// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Items/KzItemFragment.h"
#include "GameplayTagContainer.h"
#include "ScriptableTasks/ScriptableAction.h"
#include "KzItemFragment_Equippable.generated.h"

class AActor;
class UStreamableRenderAsset;
class UKzEquipmentComponent;
class UKzItemComponent;

/** Defines how the item is represented visually when equipped. */
UENUM(BlueprintType)
enum class EKzEquipmentSpawnMode : uint8
{
	SpawnActor,
	SpawnMesh
};

/** Defines the rules and visuals for equipping this item in the character's hands/slots. */
UCLASS(DisplayName = "Equippable")
class KZGAMEPLAY_API UKzItemFragment_Equippable : public UKzItemFragment
{
	GENERATED_BODY()

public:
	UKzItemFragment_Equippable();

	/** If true, the system will attempt to equip this item immediately upon picking it up, bypassing the backpack if possible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bAutoEquip = false;

	/** The specific equipment slot this item goes into (e.g., Equipment.Slot.Hand.Right). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGameplayTag TargetSlot;

	/**
	 * If set, the item will visually attach to this socket instead of the Layout's default socket for the TargetSlot.
	 * Useful for items that occupy the hands logically but sit on the shoulder or use a special IK socket.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FName SocketOverride = NAME_None;

	/**
	 * If true, the system will NOT use the default AttachToComponent.
	 * Instead, it will call PerformCustomAttach/Detach on the item's ItemComponent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bUseCustomAttachment = false;

	/** How this item should be instantiated when equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EKzEquipmentSpawnMode EquipmentSpawnMode = EKzEquipmentSpawnMode::SpawnActor;

	/** If true, uses a different Actor class when equipped instead of the default WorldActorClass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnActor", EditConditionHides))
	bool bOverrideEquipmentActor = false;

	/** The specialized actor class to spawn when equipped (e.g., a weapon with shooting logic). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnActor && bOverrideEquipmentActor", EditConditionHides))
	TSoftClassPtr<AActor> EquipmentActorClass;

	/** If true, completely disables physics collision on the spawned actor while equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	bool bDisableCollisionOnEquip = true;

	/** The mesh to attach to the character. Can be a UStaticMesh or USkeletalMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "EquipmentSpawnMode == EKzEquipmentSpawnMode::SpawnMesh", EditConditionHides, AllowedClasses = "/Script/Engine.StaticMesh, /Script/Engine.SkeletalMesh"))
	TSoftObjectPtr<UStreamableRenderAsset> EquipmentMesh;

	/** Transform offset applied when the item is attached to an equipment socket. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FTransform AttachmentOffset;

	/** Tags automatically granted to the Owner's Ability System while this item is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGameplayTagContainer EquippedTags;

	/** Action to execute when the item is equipped (e.g., granting an active ability). */
	UPROPERTY(EditAnywhere, Category = "Events")
	FScriptableAction OnEquippedAction;

	/** Helper to get the correct class to spawn when equipping as an actor. */
	TSoftClassPtr<AActor> GetEquippedActorClass(const TSoftClassPtr<AActor>& DefaultWorldClass) const
	{
		return bOverrideEquipmentActor ? EquipmentActorClass : DefaultWorldClass;
	}
};