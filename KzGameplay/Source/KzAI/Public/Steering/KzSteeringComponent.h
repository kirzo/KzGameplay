// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Steering/KzSteeringAgent.h"
#include "Containers/KzPriorityStack.h"
#include "GameplayTagContainer.h"
#include "KzSteeringComponent.generated.h"

class UKzSteeringProfile;
class UKzSteeringBehavior;

/** Represents a logical layer of steering behaviors in the priority stack. */
USTRUCT(BlueprintType)
struct FKzSteeringLayer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steering")
	FGameplayTag LayerTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steering")
	int32 Priority = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKzSteeringBehavior>> Behaviors;
};

/**
 * Evaluates active steering layers by priority and applies the resulting force as movement input.
 */
UCLASS(ClassGroup = (KzAI), meta = (BlueprintSpawnableComponent))
class KZAI_API UKzSteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKzSteeringComponent();

	/** The maximum accumulated force this agent can produce per frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float MaxSteeringForce = 2000.0f;

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent Interface

	//~ Begin UObject Interface
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	//~ End UObject Interface

	/** Instantiates a full profile and pushes it into the priority stack. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Steering")
	void PushProfile(UKzSteeringProfile* Profile, FGameplayTag LayerTag, int32 Priority);

	/**
	 * Instantiates a layer from a preconfigured behavior object.
	 * The passed behavior is duplicated internally to take ownership and prevent GC issues.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Steering")
	void PushBehavior(UKzSteeringBehavior* Behavior, FGameplayTag LayerTag, int32 Priority);

	/** Removes an entire layer from the priority stack. */
	UFUNCTION(BlueprintCallable, Category = "KzAI|Steering")
	void RemoveLayer(FGameplayTag LayerTag);

protected:
	/** Finds and caches the AgentInterface if it hasn't been cached yet. */
	void EnsureAgentInterface();

	/** The native priority stack managing our layers. */
	Kz::TPriorityStack<FKzSteeringLayer, false, FGameplayTag, false> LayerStack;

	/** Cached pointer to the interface. */
	IKzSteeringAgent* AgentInterface;
};