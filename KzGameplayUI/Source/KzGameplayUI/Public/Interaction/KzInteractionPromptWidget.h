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
 * The owning player's interaction prompt: shows the Prompt Text of the interactable their pawn's interactor focuses, and hides while
 * there is none or an interaction is running. Make it full screen, with Prompt in a Canvas Panel: anchored at the center, it moves over
 * the interactable; without bFollowInteractable it stays where it is laid out. The button goes inside Prompt, as a KzTextBlock with
 * {Input.Interact}, a KzInputIcon with an Input Tag or a KzInputKeysWidget.
 */
UCLASS(Abstract)
class KZGAMEPLAYUI_API UKzInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Moved over the focused interactable, and collapsed while there is none. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidget))
	TObjectPtr<UWidget> Prompt;

	/** Gets the interactable's Prompt Text with its arguments ({Item}...), and is disabled while the interactable cannot be used. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	/** Moves Prompt over the focused interactable. Off, Prompt stays where it is laid out, such as a corner of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bFollowInteractable = true;

	/** Added to the interactable's location, in world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bFollowInteractable"))
	FVector WorldOffset = FVector::ZeroVector;

	/** Keeps the prompt inside the screen when the interactable is near an edge or off it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bFollowInteractable"))
	bool bKeepOnScreen = true;

	/** Distance from the screen edges the prompt keeps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bFollowInteractable && bKeepOnScreen"))
	float EdgeMargin = 50.0f;

	/** The prompt appears, its text already set. By default Prompt becomes visible: an override that animates it in calls the parent first. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnShowPrompt();

	/** The prompt goes away. By default Prompt collapses: an override that animates it out leaves it visible, and it keeps following the interactable meanwhile. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnHidePrompt();

	/** The prompt changed: another interactable, none, or the same one becoming usable or not, with why not. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptChanged(UKzInteractableComponent* Interactable, bool bAvailable, FGameplayTag Reason);

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TWeakObjectPtr<APawn> InteractorPawn;
	TWeakObjectPtr<UKzInteractorComponent> Interactor;

	/** The focused interactable the prompt shows, if any. */
	TWeakObjectPtr<UKzInteractableComponent> Shown;

	/** The last interactable shown, which Prompt follows while visible, also while it animates out. */
	TWeakObjectPtr<UKzInteractableComponent> Anchor;

	/** Kept apart from Shown: an interactable destroyed while shown still has to hide the prompt. */
	bool bPromptShown = false;
	bool bShownAvailable = true;

	/** The focused interactable's Prompt Text as last formatted, to format it again only when it changes. */
	FText ShownPromptText;
};
