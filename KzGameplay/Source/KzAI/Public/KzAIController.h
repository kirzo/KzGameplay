// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "KzAIController.generated.h"

class UKzAbilitySystemComponent;
class UKzFactionComponent;
class UKzStateTreeComponent;

/**
 * Base AI Controller for the KzAI framework. Runs a StateTree: usually the one the pawn's On Possessed By AI action gives it with
 * Run StateTree, so how a character behaves travels with the character, or else one set on the controller itself.
 */
UCLASS()
class KZAI_API AKzAIController : public AAIController
{
	GENERATED_BODY()

public:
	AKzAIController(const FObjectInitializer& ObjectInitializer);

	/** Returns the attached StateTree Component. */
	UFUNCTION(BlueprintPure, Category = "KzAI")
	UKzStateTreeComponent* GetStateTreeComponent() const { return StateTreeComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KzAI")
	TObjectPtr<UKzStateTreeComponent> StateTreeComponent;

	/** The faction of the pawn it drives: the same pawn can be an ally or an enemy by who drives it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KzAI")
	TObjectPtr<UKzFactionComponent> FactionComponent;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/** Hands the pawn's gameplay events to the StateTree, with their data as payload, so transitions can wait on them: a hit, a reaction. */
	UFUNCTION()
	void HandlePawnGameplayEvent(const FGameplayTag& Tag, FGameplayEventData EventData);

	TWeakObjectPtr<UKzAbilitySystemComponent> PawnAbilitySystem;
};