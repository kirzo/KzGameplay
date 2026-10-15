// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"
#include "Interaction/KzInteractorComponent.h"

class FKzGameplayDebuggerCategory : public FGameplayDebuggerCategory
{
public:
	FKzGameplayDebuggerCategory();

	/** Creates an instance of this category. */
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	/** Called on the Game Thread to gather data from the selected Actor. */
	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;

	/** Called to draw the HUD and 3D primitives. */
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

protected:
	TWeakObjectPtr<UKzInteractorComponent> Interactor;
	TArray<FKzInteractionDebugCandidate> CandidatesData;

	/** Toggles the global rendering of all interactables in the subsystem. */
	void ToggleShowAllInteractables();

	bool bShowAllInteractables;
};
#endif