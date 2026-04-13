// Copyright 2026 kirzo

#include "StateTree/Conditions/KzStateTreeCondition_GameplayTags.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

//----------------------------------------------------------------------//
//  FKzStateTreeActorHasTagsCondition
//----------------------------------------------------------------------//

bool FKzStateTreeActorHasTagsCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.TargetActor || InstanceData.TagsToCheck.IsEmpty())
	{
		// Fail gracefully if no actor is bound or the parameter container is completely empty.
		return false ^ bInvert;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstanceData.TargetActor);
	if (!ASC)
	{
		// No ASC means no dynamic tags to check against.
		return false ^ bInvert;
	}

	// Retrieve all current tags from the ASC.
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	bool bResult = false;
	switch (MatchType)
	{
	case EGameplayContainerMatchType::Any:
		bResult = bExactMatch ? OwnedTags.HasAnyExact(InstanceData.TagsToCheck) : OwnedTags.HasAny(InstanceData.TagsToCheck);
		break;
	case EGameplayContainerMatchType::All:
		bResult = bExactMatch ? OwnedTags.HasAllExact(InstanceData.TagsToCheck) : OwnedTags.HasAll(InstanceData.TagsToCheck);
		break;
	default:
		ensureMsgf(false, TEXT("Unhandled match type."));
	}

	return bResult ^ bInvert;
}

//----------------------------------------------------------------------//
//  FKzStateTreeActorMatchTagQueryCondition
//----------------------------------------------------------------------//

bool FKzStateTreeActorMatchTagQueryCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.TargetActor || TagQuery.IsEmpty())
	{
		// Fail gracefully if no actor is bound or the query is completely empty.
		return false ^ bInvert;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstanceData.TargetActor);
	if (!ASC)
	{
		// No ASC means no dynamic tags to check against.
		return false ^ bInvert;
	}

	// Retrieve all current tags from the ASC and run the complex query.
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const bool bMatches = TagQuery.Matches(OwnedTags);

	return bMatches ^ bInvert;
}