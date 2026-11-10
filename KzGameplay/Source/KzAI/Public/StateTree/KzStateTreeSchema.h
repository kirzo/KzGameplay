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
 * Inherits the standard Unreal AI Component Schema and automatically injects KzAI specific components: the pawn's steering, and as
 * SensorComponent its Sensor.Perception sensor.
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta = (DisplayName = "KzAI Schema", CommonSchema))
class KZAI_API UKzStateTreeSchema : public UStateTreeAIComponentSchema
{
	GENERATED_BODY()

public:
	UKzStateTreeSchema(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostLoad() override;

protected:
	virtual void SetContextData(FContextDataSetter& ContextDataSetter, bool bLogErrors) const override;

private:
	/** The steering and sensor components are optional, so a tree runs on a pawn without them, such as one moved by the navmesh. */
	void MakeComponentsOptional();
};