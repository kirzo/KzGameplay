// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"
#include "Combat/KzComboInstance.h"

class UKzComboGraph;

/** How attacks pick their target: live, whom an attack would pick now with the stick as it is, or the last attack's search. */
class FKzTargetingDebuggerCategory : public FGameplayDebuggerCategory
{
public:
	FKzTargetingDebuggerCategory();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

private:
	void ToggleLive();

	bool bLive = true;

	/** Whose targeting it shows: the selected actor, or the player's pawn. */
	TWeakObjectPtr<AActor> User;
	bool bHasSensor = false;

	/** Whether it attacked, and with what: the live search takes that combo and attack too. */
	bool bAttacked = false;
	TWeakObjectPtr<const UKzComboGraph> Combo;
	int32 Node = INDEX_NONE;

	/** The search shown, and how long ago the last attack made it. */
	FKzTargetingSearch Search;
	bool bSearched = false;
	TWeakObjectPtr<AActor> GivenTarget;
	double Age = 0.0;
};
#endif