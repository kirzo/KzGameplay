// Copyright 2026 kirzo

#include "KzGameplayUILibrary.h"
#include "Input/KzUIIconSource_InputTag.h"
#include "KzInputIcon.h"

void UKzGameplayUILibrary::SetInputIconTag(UKzInputIcon* Icon, FGameplayTag InputTag)
{
	if (Icon)
	{
		FKzUIIconSource_InputTag Source;
		Source.InputTag = InputTag;
		Icon->SetSource(TInstancedStruct<FKzUIIconSource>::Make<FKzUIIconSource_InputTag>(Source));
	}
}