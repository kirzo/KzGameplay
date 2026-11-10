// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Components/KzComponentReference.h"
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

	virtual void PostLoad() override;

	/** The pawn's sensor the tree gets as SensorComponent, by name: what its AI perceives. None gives the first one it has. */
	UPROPERTY(EditAnywhere, Category = "Defaults", meta = (AllowedClasses = "/Script/KzGameplay.KzSensorComponent"))
	FKzComponentReference Sensor;

protected:
	virtual void SetContextData(FContextDataSetter& ContextDataSetter, bool bLogErrors) const override;

private:
	/** The steering and sensor components are optional, so a tree runs on a pawn without them, such as one moved by the navmesh. */
	void MakeComponentsOptional();
};