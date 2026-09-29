#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Only include EOS headers if SDK is available
#if WITH_EOS_SDK
#include "eos_sdk.h"
#include "eos_logging.h"
#include "eos_rtc_types.h"
#include <string>

// Platform-specific EOS includes
#ifdef _WIN32
#include "Windows/eos_windows.h"
#endif
#endif // WITH_EOS_SDK

/**
 * The main module class for the EpicAdapter.
 * Inherits from IModuleInterface to hook into engine startup and shutdown.
 * Handles EOS SDK initialization before RHI when SDK is available.
 */
class FEpicAdapterModule : public IModuleInterface
{
public:

	/**
	 * Called when the module is loaded into memory.
	 * Due to the "PostConfigInit" LoadingPhase, this will execute before the RHI is created.
	 * Initializes EOS SDK here if available.
	 */
	virtual void StartupModule() override;

	/**
	 * Called before the module is unloaded from memory.
	 * Shuts down EOS SDK if it was initialized.
	 */
	virtual void ShutdownModule() override;

#if WITH_EOS_SDK
	/**
	 * Gets the EOS platform handle (static access)
	 * Only available when EOS SDK is enabled
	 */
	static EOS_HPlatform GetPlatformHandle() { return PlatformHandle; }

	/**
	 * Check if EOS is initialized and ready
	 */
	static bool IsEOSInitialized() { return PlatformHandle != nullptr; }

private:
	// EOS Configuration
	bool bUseAppDataFolder = true;
	TArray<FString> CacheSubDirs = { TEXT("Redline"), TEXT("EOSCache") };
	bool EnableLogStream = true;

	// EOS Functions
	bool InitializeEOSService();
	bool InitializeEOSPlatformInstance();
	FString GetCacheDirectory() const;
	void ConfigureEOSLogger();
	void ShutdownEOSService();

	// Static callback for EOS logging
	static void ProcessLogConfiguration(const EOS_LogMessage* Message);

	// EOS Platform Handle
	static EOS_HPlatform PlatformHandle;

#else
	// Stub functions when EOS SDK is not available
public:
	static void* GetPlatformHandle() { return nullptr; }
	static bool IsEOSInitialized() { return false; }

#endif // WITH_EOS_SDK
};