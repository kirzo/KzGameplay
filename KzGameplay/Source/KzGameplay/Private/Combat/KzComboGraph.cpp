// Copyright 2026 kirzo

#include "Combat/KzComboGraph.h"
#include "Animation/KzAnimationSource.h"
#include "Combat/KzComboNodes.h"
#include "UObject/UObjectHash.h"

#define LOCTEXT_NAMESPACE "KzComboGraph"

#if WITH_EDITOR
bool UKzComboGraph::MakeNodeForAsset(UObject& Asset, TInstancedStruct<FKzGraphNode>& OutNode) const
{
	TArray<UClass*> Sources;
	GetDerivedClasses(UKzAnimationSource::StaticClass(), Sources);
	for (UClass* Class : Sources)
	{
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}

		// The schema gives the new node its own copy of the source.
		UKzAnimationSource* Source = NewObject<UKzAnimationSource>(GetTransientPackage(), Class);
		if (Source->SetAsset(Asset))
		{
			FKzComboNode_Attack Attack;
			Attack.Animation = Source;
			OutNode.InitializeAs<FKzComboNode_Attack>(Attack);
			return true;
		}
	}
	return false;
}

const UScriptStruct* UKzComboGraph::GetNodeStruct() const
{
	return FKzComboNode::StaticStruct();
}

bool UKzComboGraph::GetRequiredEntries(TArray<FName>& OutEntries) const
{
	for (const FGameplayTag Input : Inputs)
	{
		if (Input.IsValid())
		{
			OutEntries.AddUnique(Input.GetTagName());
		}
	}
	return true;
}

const UScriptStruct* UKzComboGraph::GetRequiredEntryStruct() const
{
	return FKzComboNode_Entry::StaticStruct();
}

void UKzComboGraph::SetRequiredEntry(FStructView Node, FName Entry) const
{
	if (FKzComboNode_Entry* EntryNode = Node.GetPtr<FKzComboNode_Entry>())
	{
		EntryNode->Input = FGameplayTag::RequestGameplayTag(Entry, false);
	}
}

void UKzComboGraph::ValidateGraph(const FKzGraphData& InData, TArray<FKzValidationIssue>& OutIssues) const
{
	if (Inputs.IsEmpty())
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("NoInputs", "Add the inputs that start and chain attacks."), NAME_None));
	}

	TSet<FGameplayTag> Listed;
	for (const FGameplayTag Input : Inputs)
	{
		bool bListed = false;
		Listed.Add(Input, &bListed);
		if (!Input.IsValid())
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, LOCTEXT("EmptyInput", "An input is empty."), NAME_None));
		}
		else if (bListed)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("DuplicateInput", "{0} is listed twice."), FText::FromName(Input.GetTagName())), NAME_None));
		}
		else if (InData.FindEntry(Input.GetTagName()) == INDEX_NONE)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("NoEntry", "{0} has no Entry."), FText::FromName(Input.GetTagName())), NAME_None));
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE