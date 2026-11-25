// Copyright 2026 kirzo

#include "Camera/ScriptableTask_SwitchToCamera.h"
#include "Camera/KzCameraDirector.h"
#include "GameFramework/Actor.h"

void UScriptableTask_SwitchToCamera::BeginTask()
{
	UKzCameraDirector* Director = UKzCameraDirector::Get(this);
	if (Director && IsValid(Camera))
	{
		PreviousGameplayCamera = Director->GetGameplayCamera();
		Director->SwitchTo(Camera, Blend, this);
		bSwitched = true;
	}

	Finish();
}

void UScriptableTask_SwitchToCamera::ResetTask()
{
	const bool bRevert = bRevertOnReset && bSwitched;
	bSwitched = false;

	UKzCameraDirector* Director = bRevert ? UKzCameraDirector::Get(this) : nullptr;
	if (!Director || !Director->IsShowing(Camera))
	{
		return;
	}

	if (AActor* Previous = PreviousGameplayCamera.Get(); Previous && Previous != Camera)
	{
		Director->SwitchTo(Previous, Blend, this);
	}
	else
	{
		Director->ReturnToGameplay(Blend, this);
	}
}

#if WITH_EDITOR
FText UScriptableTask_SwitchToCamera::GetDisplayTitle() const
{
	FString CameraName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_SwitchToCamera, Camera), CameraName))
	{
		CameraName = Camera ? Camera->GetActorLabel() : TEXT("None");
	}
	return FText::Format(INVTEXT("Switch To {0}"), FText::FromString(CameraName));
}
#endif