// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/ItemInstance.h"
#include "Misc/KzTransformSource.h"
#include "ItemComponent.generated.h"

class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemPickedUpDelegate, AActor*, NewOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemCustomAttachDelegate, AActor*, Equipper, FKzTransformSource, AttachSource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCustomDetachDelegate, AActor*, Equipper);

/**
 * Add this component to any Actor in the world to make it a pickable item.
 * It holds the definition and quantity, and routes the pickup logic to the interactor's inventory or equipment.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** The data asset defining what this item is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<const UItemDefinition> ItemDef;

	/** The amount of this item contained in this physical actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity;

	/**
	 * If true, the item will simulate physics when dropped or unequipped.
	 * If false, it will auto-detect its owner's root component physics state on BeginPlay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Physics")
	bool bSimulatePhysics = false;

	/** Fired exactly before this item is equipped or placed in the inventory. */
	UPROPERTY(BlueprintAssignable, Category = "Item|Events")
	FOnItemPickedUpDelegate OnPickedUp;

	/**
	 * Fired by the EquipmentComponent to handle custom attachment logic
	 * if the ItemDefinition has bUseCustomAttachment set to true.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Item|Equipment")
	FOnItemCustomAttachDelegate OnCustomAttach;

	/**
	 * Fired by the EquipmentComponent to handle custom detachment logic
	 * if the ItemDefinition has bUseCustomAttachment set to true.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Item|Equipment")
	FOnItemCustomDetachDelegate OnCustomDetach;

public:
	UItemComponent();

	FItemInstance ToItemInstance() const
	{
		return FItemInstance(ItemDef, Quantity, GetOwner());
	}

	/**
	 * Call this function when a character interacts with this item's owner.
	 * @param Interactor The actor trying to pick up this item.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Item")
	void HandleInteraction(AActor* Interactor);

protected:
	virtual void BeginPlay() override;
};