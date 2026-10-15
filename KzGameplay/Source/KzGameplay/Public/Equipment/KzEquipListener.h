// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "KzEquipListener.generated.h"

class AActor;
class UKzEquipmentComponent;
class UKzItemDefinition;

/** What an item's equip listeners hear when it is equipped or put away. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzEquipEvent
{
	GENERATED_BODY()

	/** The actor wearing the equipment. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UKzEquipmentComponent> Equipment = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FGameplayTag SlotID;

	/** The item, by its definition: by the time a change replicates, clients no longer have the instance that was put away. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<const UKzItemDefinition> Item = nullptr;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UKzEquipListener : public UInterface
{
	GENERATED_BODY()
};

/**
 * An item fragment that reacts to its item being equipped and put away, such as a pose, a sound or an effect.
 * The item's Equippable fragment tells it on every machine but a dedicated server, as the equipment shows the item, so list Equippable
 * among its required fragments. Unlike OnEquippedAction, which runs on the server only.
 */
class KZGAMEPLAY_API IKzEquipListener
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Equipment")
	void OnEquipped(const FKzEquipEvent& Event) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Equipment")
	void OnUnequipped(const FKzEquipEvent& Event) const;
};