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

	auto AddNodeIssue = [&InData, &OutIssues](int32 Node, EKzValidationSeverity Severity, const FText& Message)
	{
		FKzValidationIssue& Issue = OutIssues.Add_GetRef(FKzValidationIssue(Severity, Message, NAME_None));
		Issue.ContextId = InData.GetNodeGuid(Node);
	};

	// The branches a node leads to straight away, and the ones those lead to in turn, with no attack between.
	auto GatherBranchesAfter = [&InData](int32 From)
	{
		TArray<int32> Found;
		TArray<int32> Pending = { From };
		while (!Pending.IsEmpty())
		{
			const int32 Node = Pending.Pop();
			for (int32 Output = 0; Output < InData.NumOutputs(Node); ++Output)
			{
				for (const FKzGraphLink& Link : InData.GetLinks(Node, Output))
				{
					if (InData.GetNode<FKzComboNode_Branch>(Link.Node) && !Found.Contains(Link.Node))
					{
						Found.Add(Link.Node);
						Pending.Add(Link.Node);
					}
				}
			}
		}
		return Found;
	};

	for (int32 Node = 0; Node < InData.NumNodes(); ++Node)
	{
		if (InData.GetNode<FKzComboNode_Branch>(Node) && GatherBranchesAfter(Node).Contains(Node))
		{
			AddNodeIssue(Node, EKzValidationSeverity::Error, LOCTEXT("BranchLoop", "Its branches lead back to it with no attack between, so a press that comes here leads nowhere."));
		}
	}

	// Before the first attack there is no hit yet and no Combo Window.
	for (const FKzGraphEntry& Entry : InData.GetEntries())
	{
		for (const int32 Node : GatherBranchesAfter(Entry.Node))
		{
			if (InData.GetNode<FKzComboNode_BranchOnHit>(Node))
			{
				AddNodeIssue(Node, EKzValidationSeverity::Warning, LOCTEXT("HitFromEntry", "Comes before any attack, so it always takes Miss."));
			}
			else if (InData.GetNode<FKzComboNode_SwitchOnWindow>(Node))
			{
				AddNodeIssue(Node, EKzValidationSeverity::Warning, LOCTEXT("WindowFromEntry", "Comes before any attack, so it always takes Default."));
			}
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE