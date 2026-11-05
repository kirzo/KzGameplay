// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "KzInteractionPromptWidget.generated.h"

class APawn;
class UKzInteractableComponent;
class UKzInteractorComponent;
class UTextBlock;

/**
 * The owning player's interaction prompt: follows the interactable their pawn's interactor focuses, with its Prompt Text, and hides
 * while there is none or an interaction is running. Make it full screen, with Prompt anchored at the center of a Canvas Panel: Prompt
 * moves over the interactable. The button goes inside Prompt, as a KzTextBlock with {Input.Interact} or a KzInputIcon with an Input Tag.
 */
UCLASS(Abstract)
class KZGAMEPLAYUI_API UKzInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Moved over the focused interactable, and collapsed while there is none. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidget))
	TObjectPtr<UWidget> Prompt;

	/** Gets the interactable's Prompt Text, and is disabled while the interactable cannot be used. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	/** Added to the interactable's location, in world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FVector WorldOffset = FVector::ZeroVector;

	/** Keeps the prompt inside the screen when the interactable is near an edge or off it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bKeepOnScreen = true;

	/** Distance from the screen edges the prompt keeps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bKeepOnScreen"))
	float EdgeMargin = 50.0f;

	/** The prompt changed: another interactable, none, or the same one becoming usable or not, with why not. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptChanged(UKzInteractableComponent* Interactable, bool bAvailable, FGameplayTag Reason);

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TWeakObjectPtr<APawn> InteractorPawn;
	TWeakObjectPtr<UKzInteractorComponent> Interactor;
	TWeakObjectPtr<UKzInteractableComponent> Shown;
	bool bShownAvailable = true;
	FText ShownText;
};
