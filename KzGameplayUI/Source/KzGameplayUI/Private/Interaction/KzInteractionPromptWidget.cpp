// Copyright 2026 kirzo

#include "Interaction/KzInteractionPromptWidget.h"
#include "Interaction/KzInteractableComponent.h"
#include "Interaction/KzInteractorComponent.h"
#include "Kismet/KzSystemLibrary.h"
#include "KzUIFunctionLibrary.h"

#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

void UKzInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Shown = nullptr;
	bShownAvailable = true;
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

	const UKzInteractorComponent* InteractorComponent = Interactor.Get();
	UKzInteractableComponent* Focus = InteractorComponent && !InteractorComponent->IsInteractingContinuously() ? InteractorComponent->GetCurrentFocus() : nullptr;
	const bool bAvailable = !Focus || InteractorComponent->IsFocusAvailable();
	const bool bFocusChanged = Focus != Shown.Get();

	if (bFocusChanged || bAvailable != bShownAvailable)
	{
		Shown = Focus;
		bShownAvailable = bAvailable;
		Prompt->SetVisibility(Focus ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (PromptText)
		{
			PromptText->SetIsEnabled(bAvailable);
		}
		OnPromptChanged(Focus, bAvailable, Focus ? InteractorComponent->GetFocusUnavailableReason() : FGameplayTag());
	}

	if (!Focus)
	{
		return;
	}

	// On every new focus, even if empty, and when Prompt Text changes. Compared with what was set: a KzTextBlock may rewrite its own
	if (PromptText && (bFocusChanged || !ShownText.IdenticalTo(Focus->PromptText)))
	{
		ShownText = Focus->PromptText;
		PromptText->SetText(ShownText);
	}

	FVector2D Position;
	if (UKzUIFunctionLibrary::ProjectWorldLocationToViewport(this, Focus->GetComponentLocation() + WorldOffset, Position, bKeepOnScreen, EdgeMargin))
	{
		Prompt->SetRenderTranslation(Position);
	}
}
