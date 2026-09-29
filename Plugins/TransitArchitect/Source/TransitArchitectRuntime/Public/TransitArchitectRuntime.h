// TransitArchitectRuntime.h — Runtime module for procedural transit infrastructure
#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTransitArchitect, Log, All);

class FTransitArchitectRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
