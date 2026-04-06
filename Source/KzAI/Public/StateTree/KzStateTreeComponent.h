// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "KzStateTreeComponent.generated.h"

/**
 * Standard StateTree component for KzAI.
 * Automatically enforces the UKzStateTreeSchema for proper context data binding.
 */
UCLASS(ClassGroup = (KzAI), meta = (BlueprintSpawnableComponent))
class KZAI_API UKzStateTreeComponent : public UStateTreeComponent
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UStateTreeSchema> GetSchema() const override;
	virtual bool SetContextRequirements(FStateTreeExecutionContext& Context, bool bLogErrors = false) override;
	virtual bool CollectExternalData(const FStateTreeExecutionContext& Context, const UStateTree* StateTree, TArrayView<const FStateTreeExternalDataDesc> Descs, TArrayView<FStateTreeDataView> OutDataViews) const override;
};