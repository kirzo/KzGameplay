// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/KzItemInstance.h"
#include "Interaction/KzInteractableInterface.h"
#include "Misc/KzTransformSource.h"
#include "GameplayTagContainer.h"
#include "KzItemComponent.generated.h"

class UKzItemDefinition;
class UKzInteractorComponent;
class UKzInteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemPickedUpDelegate, AActor*, NewOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemCustomAttachDelegate, AActor*, Equipper, FKzTransformSource, AttachSource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCustomDetachDelegate, AActor*, Equipper);

/**
 * Add this component to any Actor in the world to make it a pickable item.
 * It holds the definition and quantity, and routes the pickup logic to the interactor's inventory or equipment.
 */
UCLASS(ClassGroup = (KzGameplay), meta = (BlueprintSpawnableComponent))
class KZGAMEPLAY_API UKzItemComponent : public UActorComponent, public IKzInteractableInterface
{
	GENERATED_BODY()

public:
	/** The live instance of the item represented by this component. Holds Definition, Quantity, and Stats. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FKzItemInstance ItemInstance;

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
	 * if the KzItemDefinition has bUseCustomAttachment set to true.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Item|Equipment")
	FOnItemCustomAttachDelegate OnCustomAttach;

	/**
	 * Fired by the EquipmentComponent to handle custom detachment logic
	 * if the KzItemDefinition has bUseCustomAttachment set to true.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Item|Equipment")
	FOnItemCustomDetachDelegate OnCustomDetach;

	/** Called by the EquipmentComponent when this item is equipped */
	void SetEquippedState(AActor* NewEquipper, FGameplayTag NewSlotID);

	/** Called by the EquipmentComponent when this item is dropped/stashed */
	void ClearEquippedState();

	/** Returns the Actor holding this item, if any. */
	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	AActor* GetEquipper() const { return EquipperActor.Get(); }

	/** Returns the slot this item is currently equipped in. Invalid if not equipped. */
	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	FGameplayTag GetEquippedSlot() const { return EquippedSlotID; }

	/**
	 * Returns the velocity of the equipper if held,
	 * or the physical velocity of the item itself if dropped.
	 */
	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	FVector GetItemVelocity() const;

protected:
	/** The actor currently holding this item. */
	TWeakObjectPtr<AActor> EquipperActor = nullptr;

	/** The specific slot this item is occupying. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGameplayTag EquippedSlotID;

public:
	UKzItemComponent();

protected:
	virtual void BeginPlay() override;

	/** Native implementation of the interaction interface */
	virtual EKzInteractionResult HandleInteraction_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) override;
};