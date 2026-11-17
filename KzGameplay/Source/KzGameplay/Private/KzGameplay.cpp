// Copyright 2026 kirzo

#include "KzGameplay.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "Combat/KzTargetingDebuggerCategory.h"
#include "Interaction/Debugger/KzGameplayDebuggerCategory.h"
#endif

#define LOCTEXT_NAMESPACE "FKzGameplayModule"

void FKzGameplayModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();

	GameplayDebuggerModule.RegisterCategory("KzInteraction", IGameplayDebugger::FOnGetCategory::CreateStatic(&FKzGameplayDebuggerCategory::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
	GameplayDebuggerModule.RegisterCategory("KzTargeting", IGameplayDebugger::FOnGetCategory::CreateStatic(&FKzTargetingDebuggerCategory::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FKzGameplayModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger::Get().UnregisterCategory("KzInteraction");
		IGameplayDebugger::Get().UnregisterCategory("KzTargeting");
		IGameplayDebugger::Get().NotifyCategoriesChanged();
	}
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FKzGameplayModule, KzGameplay)