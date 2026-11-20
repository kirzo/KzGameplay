// Copyright 2026 kirzo

#include "Interaction/Debugger/KzGameplayDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "KzDrawDebugHelpers.h"
#include "Interaction/KzInteractionSubsystem.h"
#include "Interaction/KzInteractableComponent.h"
#include "Engine/World.h"
#include "Math/ColorList.h"

namespace
{
	/** The front of an interactable that limits its approach, as an arrow out of it. */
	void DrawFront(const UWorld* World, const UKzInteractableComponent& Interactable, const FColor& Color)
	{
		if (Interactable.bLimitApproach)
		{
			const FVector Location = Interactable.GetComponentLocation();
			DrawDebugDirectionalArrow(World, Location, Location + Interactable.GetForwardVector() * 40.0f, 15.0f, Color, false, 0.0f, SDPG_World, 1.5f);
		}
	}

	// From the color list, so the panel's {orchid} and {turquoise} match the lines
	const FColor NearbyColor = FColorList::Orchid;
	const FColor HysteresisColor = FColorList::Turquoise;

	/** How far beyond Zone an interactable in range stays in range for the interactor. */
	void DrawHysteresisBand(const UWorld* World, const UKzInteractableComponent& Interactable, const FKzShapeInstance& Zone, float Hysteresis)
	{
		if (Hysteresis > 0.0f)
		{
			DrawDebugShape(World, Interactable.GetComponentLocation(), Interactable.GetComponentQuat(), Zone + Hysteresis, HysteresisColor, false, 0.0f, SDPG_World, 1.0f);
		}
	}

	/** The shape an interactable is noticed from: its own grown by its Nearby Margin, thicker and with its band while the interactor is nearby. */
	void DrawNearbyZone(const UWorld* World, const UKzInteractableComponent& Interactable, bool bNearby, float Hysteresis)
	{
		if (Interactable.NearbyMargin > 0.0f)
		{
			const FKzShapeInstance Zone = Interactable.Shape + Interactable.NearbyMargin;
			DrawDebugShape(World, Interactable.GetComponentLocation(), Interactable.GetComponentQuat(), Zone, NearbyColor, false, 0.0f, SDPG_World, bNearby ? 2.0f : 1.0f);
			if (bNearby)
			{
				DrawHysteresisBand(World, Interactable, Zone, Hysteresis);
			}
		}
	}
}

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
	NearbyData.Empty();
	NearbyFocus = nullptr;

	if (DebugActor)
	{
		if (UKzInteractorComponent* InteractorComp = DebugActor->FindComponentByClass<UKzInteractorComponent>())
		{
			Interactor = InteractorComp;
			CandidatesData = Interactor->LastDebugCandidates;
			for (UKzInteractableComponent* Nearby : Interactor->GetNearbyInteractables())
			{
				NearbyData.Add(Nearby);
			}
			NearbyFocus = Interactor->GetNearbyFocus();
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
			const TSet<UKzInteractableComponent*>& AllInteractables = Subsystem->GetAllRegisteredInteractables();

			CanvasContext.Printf(TEXT("{cyan}Global Registered Interactables: {yellow}%d"), AllInteractables.Num());

			// Legend for the color coding
			CanvasContext.Printf(TEXT("  {magenta}Magenta {white}= Full (Max Capacity)"));
			CanvasContext.Printf(TEXT("  {orange}Orange {white}= Dynamic"));
			CanvasContext.Printf(TEXT("  {cyan}Cyan {white}= Static"));
			CanvasContext.Printf(TEXT("  {orchid}Orchid {white}= Nearby Margin"));
			CanvasContext.Printf(TEXT("  {white}Arrow = Front (Limited Approach)"));

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
					DrawFront(World, *Interactable, DrawColor);
					DrawNearbyZone(World, *Interactable, false, 0.0f);
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
	CanvasContext.Printf(TEXT("{turquoise}Hysteresis: {white}%.0f cm"), Interactor->Hysteresis);
	CanvasContext.Printf(TEXT("{orchid}Nearby: {yellow}%d{white}, shown: %s"), NearbyData.Num(), NearbyFocus.IsValid() ? *GetNameSafe(NearbyFocus->GetOwner()) : TEXT("none"));

	DrawDebugShape(World, Interactor->GetComponentLocation(), Interactor->GetComponentQuat(), Interactor->Shape, FColor::Blue, false, 0.0f, SDPG_World, 2.0f);

	for (const TWeakObjectPtr<UKzInteractableComponent>& Nearby : NearbyData)
	{
		if (const UKzInteractableComponent* NearbyComp = Nearby.Get())
		{
			DrawNearbyZone(World, *NearbyComp, true, Interactor->Hysteresis);
			if (Nearby == NearbyFocus)
			{
				DrawDebugString(World, NearbyComp->GetComponentLocation() + FVector(0, 0, 60.0f), TEXT("Nearby, shown"), nullptr, NearbyColor, 0.0f);
			}
		}
	}

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
		DrawFront(World, *InteractableComp, DrawColor);
		DrawHysteresisBand(World, *InteractableComp, InteractableComp->Shape, Interactor->Hysteresis);
		if (!NearbyData.Contains(Cand.Interactable))
		{
			DrawNearbyZone(World, *InteractableComp, false, Interactor->Hysteresis);
		}

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