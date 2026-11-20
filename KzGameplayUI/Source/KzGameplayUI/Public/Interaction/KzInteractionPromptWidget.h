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

UENUM(BlueprintType)
enum class EKzInteractionPromptPlacement : uint8
{
	/** Over the focused interactable. */
	World,
	/** Always at Screen Position. */
	Screen
};

/**
 * The owning player's interaction prompt: shows the Prompt Text of the interactable their pawn's interactor focuses or, before anything
 * is focused, of the best one nearby, greyed and without keys. Hides while there is neither or an interaction is running. Make it full
 * screen, with Prompt in a Canvas Panel, whose anchors Placement sets, also in the designer. The keys go inside Prompt, as a
 * KzInputKeysWidget, a KzTextBlock with {Input.Interact} or a KzInputIcon with an Input Tag, named Keys.
 */
UCLASS(Abstract)
class KZGAMEPLAYUI_API UKzInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPlacement(EKzInteractionPromptPlacement NewPlacement);

	/** Whether the prompt shows something only nearby, not focused yet. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsShowingNearby() const { return bShownNearby; }

protected:
	/** Placed by Placement, and collapsed while there is nothing to show. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidget))
	TObjectPtr<UWidget> Prompt;

	/** Gets the interactable's Prompt Text with its arguments ({Item}...), and is disabled while the interactable cannot be used or is only nearby. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	/** Hidden while the prompt shows something only nearby, keeping its space so nothing moves when it gets focused. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Keys;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EKzInteractionPromptPlacement Placement = EKzInteractionPromptPlacement::World;

	/** Where Prompt's alignment point goes, as a fraction of the screen: 0, 0 is the top left corner and 1, 1 the bottom right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (EditCondition = "Placement == EKzInteractionPromptPlacement::Screen"))
	FVector2D ScreenPosition = FVector2D(0.85, 0.85);

	/** Added to the interactable's location, in world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "Placement == EKzInteractionPromptPlacement::World"))
	FVector WorldOffset = FVector::ZeroVector;

	/** Keeps the prompt inside the screen when the interactable is near an edge or off it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "Placement == EKzInteractionPromptPlacement::World"))
	bool bKeepOnScreen = true;

	/** Distance from the screen edges the prompt keeps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "Placement == EKzInteractionPromptPlacement::World && bKeepOnScreen"))
	float EdgeMargin = 50.0f;

	/** The prompt appears, its text already set. By default Prompt becomes visible: an override that animates it in calls the parent first. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnShowPrompt();

	/** The prompt goes away. By default Prompt collapses: an override that animates it out leaves it visible, and it keeps following the interactable meanwhile. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnHidePrompt();

	/** The prompt changed: another interactable, none, the same one getting focused, or becoming usable or not, with why not. Nearby only, it is not available. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptChanged(UKzInteractableComponent* Interactable, bool bAvailable, FGameplayTag Reason);

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Anchors Prompt at the screen center, where World placement moves it from, or at Screen Position. */
	void ApplyPlacement();

	TWeakObjectPtr<APawn> InteractorPawn;
	TWeakObjectPtr<UKzInteractorComponent> Interactor;

	/** The interactable the prompt shows, focused or nearby, if any. */
	TWeakObjectPtr<UKzInteractableComponent> Shown;

	/** The last interactable shown, which Prompt follows while visible, also while it animates out. */
	TWeakObjectPtr<UKzInteractableComponent> Anchor;

	/** Kept apart from Shown: an interactable destroyed while shown still has to hide the prompt. */
	bool bPromptShown = false;
	bool bShownAvailable = true;
	bool bShownNearby = false;

	/** What Keys is when shown, as laid out. */
	ESlateVisibility KeysVisibility = ESlateVisibility::Visible;

	/** The shown interactable's Prompt Text as last formatted, to format it again only when it changes. */
	FText ShownPromptText;
};
