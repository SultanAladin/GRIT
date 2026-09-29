#include "RockFormationEditor.h"
#include "RockFormationDetails.h"
#include "RockFormationActor.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FRockFormationEditorModule"

void FRockFormationEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomClassLayout(
		ARockFormationActor::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FRockFormationDetails::MakeInstance));
}

void FRockFormationEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(
			ARockFormationActor::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRockFormationEditorModule, RockFormationEditor)
