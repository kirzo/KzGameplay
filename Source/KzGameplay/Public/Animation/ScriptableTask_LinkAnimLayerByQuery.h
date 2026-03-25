// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "Core/KzDatabase.h"
#include "ScriptableTask_LinkAnimLayerByQuery.generated.h"

class AActor;
class UAnimInstance;

/**
 * Resolves an Animation Layer class dynamically using the Target Actor's Database Component,
 * then links it to its Skeletal Mesh (preferring the UKzAnimLayerComponent if available).
 */
UCLASS(NotBlueprintable, DisplayName = "Link Anim Layers (By Query)", meta = (TaskCategory = "Animation"))
class KZGAMEPLAY_API UScriptableTask_LinkAnimLayerByQuery : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The actor that owns the Skeletal Mesh, the Anim Layer Component, and the Database Component. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<AActor> TargetActor;

	/** The query used to find the best matching animation layer in the actor's databases. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	FKzDatabaseQuery Query;

	/** The priority of the layer when pushed to the AnimLayerComponent. Ignored if the component is missing. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	int32 LayerPriority = 1;

	/** If true, the resolved layer will be unlinked when this task is reset. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	bool bRevertOnReset = true;

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

private:
	/**
	 * Caches the resolved class during BeginTask.
	 * This ensures that ResetTask unlinks the exact same class, even if the Database changes later.
	 */
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> ResolvedLayerClass = nullptr;
};