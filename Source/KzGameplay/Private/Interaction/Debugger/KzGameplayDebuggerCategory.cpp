// Copyright 2026 kirzo

#include "Interaction/Debugger/KzGameplayDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "KzDrawDebugHelpers.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractableComponent.h"
#include "Engine/World.h"

FKzGameplayDebuggerCategory::FKzGameplayDebuggerCategory()
{
	bShowOnlyWithDebugActor = false;
	Interactor = nullptr;
	bShowAllInteractables = false;

	BindKeyPress(EKeys::One.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &FKzGameplayDebuggerCategory::ToggleShowAllInteractables, EGameplayDebuggerInputMode::Local);
}

TSharedRef<FGameplayDebuggerCategory> FKzGameplayDebuggerCategory::MakeInstance()
{
	return MakeShareable(new FKzGameplayDebuggerCategory());
}

void FKzGameplayDebuggerCategory::ToggleShowAllInteractables()
{
	bShowAllInteractables = !bShowAllInteractables;
}

void FKzGameplayDebuggerCategory::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	Interactor = nullptr;
	CandidatesData.Empty();

	if (DebugActor)
	{
		if (UKzInteractorComponent* InteractorComp = DebugActor->FindComponentByClass<UKzInteractorComponent>())
		{
			Interactor = InteractorComp;
			CandidatesData = Interactor->LastDebugCandidates;
		}
	}
}

void FKzGameplayDebuggerCategory::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	UWorld* World = CanvasContext.World.Get();
	if (!World) return;

	CanvasContext.Printf(TEXT("{white}[SHIFT + 1] {green}Draw All Interactables: {yellow}%s"), bShowAllInteractables ? TEXT("ON") : TEXT("OFF"));
	CanvasContext.Printf(TEXT(""));

	if (bShowAllInteractables)
	{
		if (UKzInteractionSubsystem* Subsystem = World->GetSubsystem<UKzInteractionSubsystem>())
		{
			const TSet<TObjectPtr<UKzInteractableComponent>>& AllInteractables = Subsystem->GetAllRegisteredInteractables();

			CanvasContext.Printf(TEXT("{cyan}Global Registered Interactables: {yellow}%d"), AllInteractables.Num());

			// Legend for the color coding
			CanvasContext.Printf(TEXT("  {magenta}Magenta {white}= Full (Max Capacity)"));
			CanvasContext.Printf(TEXT("  {orange}Orange {white}= Dynamic"));
			CanvasContext.Printf(TEXT("  {cyan}Cyan {white}= Static"));

			for (UKzInteractableComponent* Interactable : AllInteractables)
			{
				if (IsValid(Interactable))
				{
					FColor DrawColor;
					if (Interactable->IsInteractionFull())
					{
						DrawColor = FColor::Magenta;
					}
					else
					{
						DrawColor = Interactable->bIsDynamicInteraction ? FColor::Orange : FColor::Cyan;
					}

					DrawDebugShape(World, Interactable->GetComponentLocation(), Interactable->GetComponentQuat(), Interactable->Shape, DrawColor, false, 0.0f, SDPG_World, 1.0f);
				}
			}
		}
	}

	if (!Interactor.IsValid())
	{
		CanvasContext.Printf(TEXT("{red}Selected Actor has no UKzInteractorComponent."));
		return;
	}

	// Draw 2D Info on the left panel
	CanvasContext.Printf(TEXT("{green}Interaction Debug for: {white}%s"), *GetNameSafe(Interactor->GetOwner()));
	CanvasContext.Printf(TEXT("Candidates Evaluated: {yellow}%d"), CandidatesData.Num());

	DrawDebugShape(World, Interactor->GetComponentLocation(), Interactor->GetComponentQuat(), Interactor->Shape, FColor::Blue, false, 0.0f, SDPG_World, 2.0f);

	for (const FKzInteractionDebugCandidate& Cand : CandidatesData)
	{
		UKzInteractableComponent* InteractableComp = Cand.Interactable.Get();
		if (!InteractableComp) continue;

		bool bIsFull = InteractableComp->IsInteractionFull();

		FColor DrawColor = Cand.bPassedFilters ? (Cand.bIsBest ? FColor::Green : FColor::Yellow) : FColor::Red;
		FString Text = Cand.bPassedFilters ? FString::Printf(TEXT("Score: %.2f"), Cand.Score) : TEXT("Filtered");

		// Append FULL warning to the floating text
		if (bIsFull)
		{
			Text += TEXT(" (FULL)");
			DrawColor = FColor::Magenta;
		}

		// Draw text
		DrawDebugString(World, Cand.Interactable->GetComponentLocation() + FVector(0, 0, 40.0f), Text, nullptr, DrawColor, 0.0f);

		// ======= DRAW THE INTERACTABLE =======
		// If the pointer is still valid, draw its bounds or specific shape
		DrawDebugShape(World, InteractableComp->GetComponentLocation(), InteractableComp->GetComponentQuat(), InteractableComp->Shape, DrawColor, false, 0.0f, SDPG_World, 2.0f);

		// If you want to show extra details on the 2D panel for the winning target:
		if (Cand.bIsBest)
		{
			CanvasContext.Printf(TEXT("  {green}Winner: {white}%s"), *GetNameSafe(Cand.Interactable->GetOwner()));
			CanvasContext.Printf(TEXT("  {green}Is Automatic: {white}%s"), InteractableComp->bIsAutomaticInteraction ? TEXT("True") : TEXT("False"));

			if (bIsFull)
			{
				CanvasContext.Printf(TEXT("  {magenta}Warning: Target is FULL"));
			}
		}

		if (Cand.bIsBest)
		{
			DrawDebugDirectionalArrow(World, Interactor->GetComponentLocation(), Cand.Interactable->GetComponentLocation(), 40.0f, FColor::Green, false, 0.0f, 0, 2.0f);
		}
	}
}

#endif