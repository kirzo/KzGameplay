// Copyright 2026 kirzo

#include "StateTree/KzStateTreeComponent.h"
#include "StateTree/KzStateTreeSchema.h"
#include "StateTreeExecutionContext.h"

TSubclassOf<UStateTreeSchema> UKzStateTreeComponent::GetSchema() const
{
	return UKzStateTreeSchema::StaticClass();
}

bool UKzStateTreeComponent::SetContextRequirements(FStateTreeExecutionContext& Context, bool bLogErrors)
{
	Context.SetCollectExternalDataCallback(FOnCollectStateTreeExternalData::CreateUObject(this, &UKzStateTreeComponent::CollectExternalData));
	return UKzStateTreeSchema::SetContextRequirements(*this, Context, bLogErrors);
}

bool UKzStateTreeComponent::CollectExternalData(const FStateTreeExecutionContext& Context, const UStateTree* StateTree, TArrayView<const FStateTreeExternalDataDesc> Descs, TArrayView<FStateTreeDataView> OutDataViews) const
{
	return UKzStateTreeSchema::CollectExternalData(Context, StateTree, Descs, OutDataViews);
}