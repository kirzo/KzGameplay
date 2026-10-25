// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Animation/KzAnimationNotifyState.h"
#include "KzCombatNotifies.generated.h"

/** While it lasts, the attack's hitboxes hit. */
UCLASS(meta = (DisplayName = "Hit Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_HitWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_HitWindow();
};

/** While it lasts, a press chains the next attack. */
UCLASS(meta = (DisplayName = "Combo Window"))
class KZGAMEPLAY_API UKzAnimNotifyState_ComboWindow : public UKzAnimationNotifyState
{
	GENERATED_BODY()

public:
	UKzAnimNotifyState_ComboWindow();
};