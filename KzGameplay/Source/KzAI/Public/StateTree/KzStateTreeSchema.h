// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "KzStateTreeSchema.generated.h"

namespace Kz::StateTree::Context
{
	const FName SteeringComponent = TEXT("SteeringComponent");
	const FName SensorComponent = TEXT("SensorComponent");
};

/**
 * Generic StateTree Schema for KzAI.
 * Inherits the standard Unreal AI Component Schema and automatically injects KzAI specific components.
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta = (DisplayName = "KzAI Schema", CommonSchema))
class KZAI_API UKzStateTreeSchema : public UStateTreeAIComponentSchema
{
	GENERATED_BODY()

public:
	UKzStateTreeSchema(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void SetContextData(FContextDataSetter& ContextDataSetter, bool bLogErrors) const override;
};