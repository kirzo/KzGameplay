// Copyright 2026 kirzo

#include "ScriptableTask_SetLightingProfile.h"
#include "KzLightingRig.h"

void UScriptableTask_SetLightingProfile::BeginTask()
{
	if (AKzLightingRig* Rig = AKzLightingRig::Get(this); Rig && Layer.Profile)
	{
		Rig->PushLayer(this, Layer, bSnap);
		PushedTo = Rig;
	}

	Finish();
}

void UScriptableTask_SetLightingProfile::ResetTask()
{
	if (bRevertOnReset && PushedTo.IsValid())
	{
		PushedTo->PopLayer(this, bSnap);
	}
	PushedTo.Reset();
}

#if WITH_EDITOR
FText UScriptableTask_SetLightingProfile::GetDisplayTitle() const
{
	return Layer.Profile ? FText::Format(INVTEXT("Set Lighting Profile {0}"), FText::FromString(Layer.Profile->GetName())) : INVTEXT("Set Lighting Profile");
}
#endif