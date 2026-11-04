// Copyright 2026 kirzo

#include "Camera/KzCameraModule.h"

UClass* UKzCameraModule::GetSlotClass() const
{
	UClass* Class = GetClass();
	while (Class->GetSuperClass() && Class->GetSuperClass() != UKzCameraModule::StaticClass())
	{
		Class = Class->GetSuperClass();
	}
	return Class;
}

UWorld* UKzCameraModule::GetWorld() const
{
	if (IsTemplate()) return nullptr;
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}
