// TransitArchitectEditor.h — Editor module for transit infrastructure detail panels
#pragma once

#include "Modules/ModuleManager.h"

class FTransitArchitectEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
