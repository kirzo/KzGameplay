// Copyright 2026 kirzo

#include "Factories/ComboGraphAssetFactory.h"
#include "Combat/KzComboGraph.h"

UComboGraphAssetFactory::UComboGraphAssetFactory()
{
	SupportedClass = UKzComboGraph::StaticClass();
}