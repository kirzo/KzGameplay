// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_LinkAnimLayers.generated.h"

class UAnimInstance;

/** Task that dynamically links an Animation Blueprint Layer to the target actor's main Skeletal Mesh. */
UCLASS(NotBlueprintable, DisplayName = "Link Anim Layers", meta = (TaskCategory = "Animation"))
class KZGAMEPLAY_API UScriptableTask_LinkAnimLayers : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The actor whose skeletal mesh will receive the anim layers. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<AActor> TargetActor;

	/** The Anim Instance class containing the layers to link (e.g., ABP_Orc_Run). */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftClassPtr<UAnimInstance> AnimLayerClass;

	/** The priority of the layer when pushed to the AnimLayerComponent. Ignored if the component is missing. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	int32 LayerPriority = 1;

	/** If true, calling Reset() on this task will unlink the layers, restoring the previous animation state. */
	UPROPERTY(EditAnywhere, Category = "Animation")
	bool bRevertOnReset = false;

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};