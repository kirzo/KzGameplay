// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "Cinematics/KzCinematicSubsystem.h"
#include "ScriptableTask_CinematicMode.generated.h"

/**
 * Keeps a player in cinematic mode while it runs, and takes it back when stopped. It never finishes by itself: it belongs in an
 * action that stops it, such as an interactable's While Action, which does when the interaction ends.
 */
UCLASS(DisplayName = "Cinematic Mode", meta = (TaskCategory = "Gameplay|Cinematic"))
class KZGAMEPLAY_API UScriptableTask_CinematicMode : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** The player's pawn or controller, such as an interaction's Instigator. */
	UPROPERTY(EditAnywhere, Category = "Cinematic", meta = (ScriptableContext))
	TObjectPtr<AActor> Player;

	UPROPERTY(EditAnywhere, Category = "Cinematic", meta = (ShowOnlyInnerProperties))
	FKzCinematicSettings Settings;

	virtual bool IsStoppable() const override { return true; }

protected:
	virtual void BeginTask() override;
	virtual void StopTask() override;

private:
	TWeakObjectPtr<APlayerController> AppliedTo;
};