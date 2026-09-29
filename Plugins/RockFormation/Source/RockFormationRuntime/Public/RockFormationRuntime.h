#pragma once

#include "Modules/ModuleManager.h"

class FRockFormationRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
