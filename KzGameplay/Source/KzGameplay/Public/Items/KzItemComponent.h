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
struct FStreamableHandle;
class UKzEquipmentComponent;
class UKzInteractorComponent;
class UKzInteractableComponent;

/** One slot holding the item: of which equipment, which slot, and the interactable it was picked up by. */
USTRUCT(BlueprintType)
struct KZGAMEPLAY_API FKzItemHolder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<UKzEquipmentComponent> Equipment = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FGameplayTag SlotID;

	/** Null when it was handed over rather than picked up: equipped by definition, from the inventory or by code. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<UKzInteractableComponent> PickedUpBy = nullptr;

	/** The actor wearing the equipment. */
	AActor* GetActor() const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemPickedUpDelegate, AActor*, NewOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemHeldDelegate, const FKzItemHolder&, Holder, FKzTransformSource, AttachSource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemReleasedDelegate, const FKzItemHolder&, Holder);

/**
 * Add this component to any Actor in the world to make it a pickable item.
 * It holds the definition and quantity, and routes the pickup logic to the interactor's inventory or equipment.
 *
 * The item has no owner: it has holders, the equipment slots holding it, as many as its Equippable's Max Holders. The equipment slots are
 * what replicates; every machine's equipment tells the item as its slots change, so the holders are the same everywhere.
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
	 * Fired on every machine as a slot starts holding the item, with where that slot holds it: an item with custom attachment hangs itself
	 * there. A slot moving it to another, as when putting it away, releases it in the one and holds it in the other.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Item|Events")
	FOnItemHeldDelegate OnHeld;

	/** Fired on every machine as a slot stops holding the item. */
	UPROPERTY(BlueprintAssignable, Category = "Item|Events")
	FOnItemReleasedDelegate OnReleased;

	/** The slots holding the item, of one equipment or of several, in no particular order. */
	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	TArray<FKzItemHolder> GetHolders() const { return Holders; }

	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	bool IsHeld() const { return !Holders.IsEmpty(); }

	/** Whether any slot of the actor's equipment holds the item. */
	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	bool IsHeldBy(const AActor* Actor) const;

	/** Whether one more slot can hold the item: fewer hold it than its Equippable's Max Holders. */
	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	bool HasRoomForHolder() const;

	/** Called by an equipment on every machine as one of its slots starts holding the item. */
	void AddHolder(const FKzItemHolder& Holder, const FKzTransformSource& AttachSource);

	/** Called by an equipment on every machine as one of its slots stops holding the item. */
	void RemoveHolder(const UKzEquipmentComponent* Equipment, FGameplayTag SlotID);

	/** The velocity of whoever holds the item, averaged over its holders, or the item's own while nobody does. */
	UFUNCTION(BlueprintPure, Category = "KzGameplay|Item")
	FVector GetItemVelocity() const;

	UKzItemComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Native implementation of the interaction interface */
	virtual bool CanInteract_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) override;
	virtual EKzInteractionResult GetInteractionResult_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) override;
	virtual void OnInteractionBegun_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable, const FKzInteraction& Interaction) override;

	/** {Item}: the item's Display Name. */
	virtual TMap<FString, FText> GetPromptArguments_Implementation(UKzInteractorComponent* Interactor, UKzInteractableComponent* Interactable) override;

private:
	/** Kept by the equipments holding the item, on every machine. */
	UPROPERTY(Transient)
	TArray<FKzItemHolder> Holders;

	/** Keeps what the item's fragments preload in memory while the actor plays. */
	TSharedPtr<FStreamableHandle> PreloadHandle;
};