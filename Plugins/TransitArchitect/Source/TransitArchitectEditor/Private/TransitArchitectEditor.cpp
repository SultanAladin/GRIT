// TransitArchitectEditor.cpp
#include "TransitArchitectEditor.h"
#include "TransitNetworkDetails.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FTransitArchitectEditorModule"

void FTransitArchitectEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		"TransitNetworkActor",
		FOnGetDetailCustomizationInstance::CreateStatic(&FTransitNetworkDetails::MakeInstance));
}

void FTransitArchitectEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("TransitNetworkActor");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTransitArchitectEditorModule, TransitArchitectEditor)
