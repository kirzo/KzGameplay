// Copyright 2026 kirzo

#include "Combat/KzCombatNotifies.h"

UKzAnimNotifyState_HitWindow::UKzAnimNotifyState_HitWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(220, 60, 50);
#endif
}

UKzAnimNotifyState_ComboWindow::UKzAnimNotifyState_ComboWindow()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 180, 90);
#endif
}