// Copyright 2026 kirzo

#include "Combat/KzTargetingDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "Animation/KzAnimationSource.h"
#include "Combat/KzComboGraph.h"
#include "Combat/KzComboNodes.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/KzInputHandlerComponent.h"
#include "Input/KzInputTags.h"
#include "Kismet/KzSystemLibrary.h"
#include "KzDrawDebugHelpers.h"
#include "Sensors/KzSensorComponent.h"
#include "Sensors/KzSensorTags.h"

namespace
{
	const FKzComboNode_Attack* FindAttack(const UKzComboGraph* Graph, int32 Node)
	{
		return Graph && Node >= 0 && Node < Graph->GetData().NumNodes() ? Graph->GetData().GetNode<FKzComboNode_Attack>(Node) : nullptr;
	}

	/** Where an attack would look now, as the combo ability reads it: the stick, what last reached the movement, or ahead. */
	FVector GetAimDirection(const AActor& Actor)
	{
		const UKzInputHandlerComponent* InputHandler = Actor.FindComponentByClass<UKzInputHandlerComponent>();
		FVector Direction = InputHandler ? InputHandler->GetIntendedInput(KzTags::Input::Move) : FVector::ZeroVector;
		const APawn* Pawn = Cast<APawn>(&Actor);
		if (Direction.IsNearlyZero() && Pawn)
		{
			Direction = Pawn->GetLastMovementInputVector();
		}
		Direction.Z = 0.0;
		return Direction.IsNearlyZero() ? Actor.GetActorForwardVector().GetSafeNormal2D() : Direction.GetSafeNormal();
	}

	FString GetAttackName(const FKzComboNode_Attack* Attack, int32 Node)
	{
		const FString Description = Attack && Attack->Animation ? Attack->Animation->GetDescription().ToString() : FString();
		return Description.IsEmpty() ? FString::Printf(TEXT("node %d"), Node) : Description;
	}
}

FKzTargetingDebuggerCategory::FKzTargetingDebuggerCategory()
{
	bShowOnlyWithDebugActor = false;
	BindKeyPress(EKeys::L.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &FKzTargetingDebuggerCategory::ToggleLive, EGameplayDebuggerInputMode::Local);
}

TSharedRef<FGameplayDebuggerCategory> FKzTargetingDebuggerCategory::MakeInstance()
{
	return MakeShareable(new FKzTargetingDebuggerCategory());
}

void FKzTargetingDebuggerCategory::ToggleLive()
{
	bLive = !bLive;
}

void FKzTargetingDebuggerCategory::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	AActor* Actor = DebugActor ? DebugActor : OwnerPC ? OwnerPC->GetPawn() : nullptr;
	User = Actor;
	Search = FKzTargetingSearch();
	bSearched = false;
	GivenTarget.Reset();
	Combo.Reset();
	Node = INDEX_NONE;

	UKzSensorComponent* Sensor = Actor ? UKzSystemLibrary::FindComponentByGameplayTag<UKzSensorComponent>(Actor, KzTags::Sensor::Targeting) : nullptr;
	bHasSensor = Sensor != nullptr;

	const FKzTargetingRecord* Record = UKzComboInstance::FindTargetingRecord(Actor);
	bAttacked = Record != nullptr;
	if (Record)
	{
		Combo = Record->Combo.Get();
		Node = Record->Node;
	}

	if (!bLive)
	{
		if (Record)
		{
			Search = Record->Search;
			bSearched = Record->bSearched;
			GivenTarget = Record->GivenTarget;
			Age = Actor->GetWorld() ? Actor->GetWorld()->GetTimeSeconds() - Record->Time : 0.0;
		}
		return;
	}

	// With the last attack, or before any, with the sensor's own shape and the closest first
	const UKzComboGraph* Graph = Combo.Get();
	const FKzComboNode_Attack* Attack = FindAttack(Graph, Node);
	const FKzComboNode_Attack NoAttack;
	const FKzTargetScoringProfile NoScoring;
	bSearched = Sensor && UKzComboInstance::SearchTargets(*Sensor, Attack ? *Attack : NoAttack, Graph ? Graph->TargetScoring : NoScoring, *Actor, GetAimDirection(*Actor), Search);
}

void FKzTargetingDebuggerCategory::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Printf(TEXT("{white}[SHIFT + L] {green}Showing: {yellow}%s"), bLive ? TEXT("live, whom an attack now would pick") : TEXT("the last attack's search"));

	UWorld* World = CanvasContext.World.Get();
	const AActor* Actor = User.Get();
	if (!World || !Actor)
	{
		CanvasContext.Printf(TEXT("{red}Select a character, or possess one."));
		return;
	}

	CanvasContext.Printf(TEXT("{green}Targeting of {white}%s"), *Actor->GetName());
	if (!bHasSensor)
	{
		CanvasContext.Printf(TEXT("{red}No sensor tagged Sensor.Targeting: attacks find no target."));
	}
	if (!bAttacked && !bLive)
	{
		CanvasContext.Printf(TEXT("{yellow}No attack yet. Attack, or switch to live."));
		return;
	}

	const UKzComboGraph* Graph = Combo.Get();
	if (Graph)
	{
		CanvasContext.Printf(TEXT("Combo {yellow}%s{white}, attack {yellow}%s"), *Graph->GetName(), *GetAttackName(FindAttack(Graph, Node), Node));
	}
	else
	{
		CanvasContext.Printf(TEXT("{yellow}No attack yet: the sensor's shape, closest first."));
	}
	if (!bLive)
	{
		CanvasContext.Printf(TEXT("%.1f s ago"), Age);
	}

	if (const AActor* Given = GivenTarget.Get())
	{
		CanvasContext.Printf(TEXT("{cyan}Target given by whoever drives the combo: {white}%s"), *Given->GetName());
		return;
	}
	if (!bSearched)
	{
		CanvasContext.Printf(TEXT("{red}No search: no targeting shape."));
		return;
	}

	const bool bFound = Search.Candidates.IsValidIndex(Search.Best);
	DrawDebugShape(World, Search.Transform.GetLocation(), Search.Transform.GetRotation(), Search.Shape, bFound ? FColor::Green : FColor::Yellow, false, 0.0f, SDPG_World, 1.0f);
	DrawDebugDirectionalArrow(World, Search.Origin, Search.Origin + Search.Direction.GetSafeNormal2D() * 150.0f, 40.0f, FColor::White, false, 0.0f, SDPG_World, 2.0f);

	const TArray<FKzTargetScorerEntry>* Entries = Graph ? &Graph->TargetScoring.Entries : nullptr;
	const bool bScored = Entries && !Entries->IsEmpty();
	CanvasContext.Printf(TEXT("Candidates {yellow}%d{white}%s"), Search.Candidates.Num(), bScored ? TEXT("") : TEXT(", no Target Scoring: the closest wins"));

	TArray<int32> Order;
	for (int32 Index = 0; Index < Search.Candidates.Num(); ++Index)
	{
		Order.Add(Index);
	}
	Order.Sort([this](int32 A, int32 B) { return Search.Candidates[A].Score > Search.Candidates[B].Score; });

	for (const int32 Index : Order)
	{
		const FKzTargetingCandidate& Candidate = Search.Candidates[Index];
		const bool bBest = Index == Search.Best;

		TArray<FString> Parts;
		if (!bScored)
		{
			Parts.Add(FString::Printf(TEXT("%.0f cm away"), -Candidate.Score));
		}
		for (int32 Entry = 0; Entry < Candidate.Entries.Num(); ++Entry)
		{
			const UKzTargetScorer* Scorer = Entries && Entries->IsValidIndex(Entry) ? (*Entries)[Entry].Scorer.Get() : nullptr;
			const FString Name = Scorer ? Scorer->GetClass()->GetName().Replace(TEXT("KzTargetScorer_"), TEXT("")) : TEXT("None");
			Parts.Add(FString::Printf(TEXT("%s %.2f -> %+.2f"), *Name, Candidate.Entries[Entry].Raw, Candidate.Entries[Entry].Added));
		}
		CanvasContext.Printf(TEXT("%s%s %s {white}%.2f  %s"), bBest ? TEXT("{green}") : TEXT("{yellow}"), bBest ? TEXT(">") : TEXT("  "), *GetNameSafe(Candidate.Actor.Get()), Candidate.Score, *FString::Join(Parts, TEXT(" | ")));

		const FColor Color = bBest ? FColor::Green : FColor::Yellow;
		if (bBest)
		{
			DrawDebugDirectionalArrow(World, Search.Origin, Candidate.Location, 40.0f, Color, false, 0.0f, SDPG_World, 3.0f);
		}
		else
		{
			DrawDebugLine(World, Search.Origin, Candidate.Location, Color, false, 0.0f, SDPG_World, 1.0f);
		}
		DrawDebugString(World, Candidate.Location + FVector(0.0, 0.0, 100.0), FString::Printf(TEXT("%.2f"), Candidate.Score), nullptr, Color, 0.0f);
	}
}
#endif