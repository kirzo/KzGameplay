// Copyright 2026 kirzo

#include "Interaction/KzInteractionPromptWidget.h"
#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractorComponent.h"
#include "Kismet/KzSystemLibrary.h"
#include "KzUIFunctionLibrary.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

void UKzInteractionPromptWidget::SetPlacement(EKzInteractionPromptPlacement NewPlacement)
{
	Placement = NewPlacement;
	ApplyPlacement();
}

void UKzInteractionPromptWidget::ApplyPlacement()
{
	if (UCanvasPanelSlot* PromptSlot = Cast<UCanvasPanelSlot>(Prompt->Slot))
	{
		const FVector2D Point = Placement == EKzInteractionPromptPlacement::Screen ? ScreenPosition : FVector2D(0.5);
		PromptSlot->SetAnchors(FAnchors(static_cast<float>(Point.X), static_cast<float>(Point.Y)));
	}
	Prompt->SetRenderTranslation(FVector2D::ZeroVector);
}

void UKzInteractionPromptWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Also in the designer, to see where a Screen prompt goes
	ApplyPlacement();
}

void UKzInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Shown = nullptr;
	Anchor = nullptr;
	bPromptShown = false;
	bShownAvailable = true;
	bShownNearby = false;
	KeysVisibility = Keys ? Keys->GetVisibility() : ESlateVisibility::Visible;
	Prompt->SetVisibility(ESlateVisibility::Collapsed);
}

void UKzInteractionPromptWidget::OnShowPrompt_Implementation()
{
	Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKzInteractionPromptWidget::OnHidePrompt_Implementation()
{
	Prompt->SetVisibility(ESlateVisibility::Collapsed);
}

void UKzInteractionPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The interactor is looked up again only when the pawn changes: possession, respawn
	APawn* Pawn = GetOwningPlayerPawn();
	if (Pawn != InteractorPawn.Get())
	{
		InteractorPawn = Pawn;
		Interactor = UKzSystemLibrary::FindComponentInActorOrController<UKzInteractorComponent>(Pawn);
	}

	UKzInteractorComponent* InteractorComponent = Interactor.Get();
	const bool bFree = InteractorComponent && !InteractorComponent->IsInteractingContinuously();
	UKzInteractableComponent* Focus = bFree ? InteractorComponent->GetCurrentFocus() : nullptr;

	// Before anything can be used, what is near enough to notice
	UKzInteractableComponent* Target = Focus ? Focus : (bFree ? InteractorComponent->GetNearbyFocus() : nullptr);
	const bool bNearby = !Focus && Target;
	const bool bShow = Target != nullptr;
	const bool bAvailable = Focus ? InteractorComponent->IsFocusAvailable() : !bNearby;
	const bool bFocusChanged = Target != Shown.Get() || bShow != bPromptShown || bNearby != bShownNearby;

	// On every new target, even if empty, and when its text changes; formatting with the arguments only then, not every frame
	const FText* SourceText = Target ? (bNearby ? &Target->GetNearbyPromptSource() : &Target->PromptText) : nullptr;
	if (SourceText && PromptText && (bFocusChanged || !ShownPromptText.IdenticalTo(*SourceText)))
	{
		ShownPromptText = *SourceText;
		PromptText->SetText(bNearby ? Target->GetNearbyPromptText(InteractorComponent) : Target->GetPromptText(InteractorComponent));
	}

	if (bFocusChanged || bAvailable != bShownAvailable)
	{
		Shown = Target;
		bShownAvailable = bAvailable;
		bShownNearby = bNearby;
		if (Target)
		{
			Anchor = Target;
		}
		if (PromptText)
		{
			PromptText->SetIsEnabled(bAvailable);
		}
		if (Keys)
		{
			Keys->SetVisibility(bNearby ? ESlateVisibility::Collapsed : KeysVisibility);
		}

		if (bShow != bPromptShown)
		{
			bPromptShown = bShow;
			if (bShow)
			{
				OnShowPrompt();
			}
			else
			{
				OnHidePrompt();
			}
		}
		OnPromptChanged(Target, bAvailable, Focus ? InteractorComponent->GetFocusUnavailableReason() : FGameplayTag());

		// This tick runs mid-paint: unmeasured, Prompt would be laid out this frame with the size of its last content, or none
		const TSharedPtr<SWidget> PromptWidget = Prompt->GetCachedWidget();
		if (bShow && PromptWidget)
		{
			PromptWidget->MarkPrepassAsDirty();
			PromptWidget->SlatePrepass(MyGeometry.Scale);
		}
	}

	FVector2D Position;
	const UKzInteractableComponent* AnchorComponent = Anchor.Get();
	if (Placement == EKzInteractionPromptPlacement::World && AnchorComponent && Prompt->IsVisible() && UKzUIFunctionLibrary::ProjectWorldLocationToViewport(this, AnchorComponent->GetComponentLocation() + WorldOffset, Position, bKeepOnScreen, EdgeMargin))
	{
		Prompt->SetRenderTranslation(Position);
	}
}
