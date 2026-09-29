// Copyright Epic Games, Inc. All Rights Reserved.

#include "EpicAdapter.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

#if WITH_EOS_SDK
#include <string>
#define ALLOW_RESERVED_OPTIONS 0  // or 1 depending on your needs
#endif

// Define a unique log category for this plugin
DEFINE_LOG_CATEGORY_STATIC(LogEpicAdapter, Log, All);

#if WITH_EOS_SDK
// Static member definition (only when EOS SDK is available)
EOS_HPlatform FEpicAdapterModule::PlatformHandle = nullptr;
#endif

void FEpicAdapterModule::StartupModule()
{
#if WITH_EOS_SDK
	UE_LOG(LogEpicAdapter, Warning, TEXT("EpicAdapter::StartupModule() - Initializing EOS before RHI (WITH_EOS_SDK=1)"));

	// Initialize EOS Service and Platform
	bool bEOSInitSuccess = InitializeEOSService() && InitializeEOSPlatformInstance();
	
	if (bEOSInitSuccess)
	{
		UE_LOG(LogEpicAdapter, Warning, TEXT("EOS Successfully initialized in pre-RHI phase!"));
	}
	else
	{
		UE_LOG(LogEpicAdapter, Error, TEXT("Failed to initialize EOS in pre-RHI phase!"));
	}
#else
	UE_LOG(LogEpicAdapter, Warning, TEXT("EpicAdapter::StartupModule() - EOS SDK not available (WITH_EOS_SDK=0)"));
	UE_LOG(LogEpicAdapter, Warning, TEXT("Plugin loaded but EOS functionality is disabled. Check your EOS SDK installation."));
#endif
}

void FEpicAdapterModule::ShutdownModule()
{
#if WITH_EOS_SDK
	UE_LOG(LogEpicAdapter, Log, TEXT("EpicAdapter::ShutdownModule() - Shutting down EOS"));
	ShutdownEOSService();
#else
	UE_LOG(LogEpicAdapter, Log, TEXT("EpicAdapter::ShutdownModule() - No EOS to shutdown"));
#endif
}

#if WITH_EOS_SDK
bool FEpicAdapterModule::InitializeEOSService()
{
	if (EnableLogStream)
	{
		ConfigureEOSLogger();
	}

	// Prepare the initialization structure
	EOS_InitializeOptions InitializeOptions = {};
	InitializeOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	InitializeOptions.AllocateMemoryFunction = nullptr;
	InitializeOptions.ReallocateMemoryFunction = nullptr;
	InitializeOptions.ReleaseMemoryFunction = nullptr;
	
	// Product information
	InitializeOptions.ProductName = "GTX";
	InitializeOptions.ProductVersion = "1.0";
	InitializeOptions.Reserved = nullptr;
	InitializeOptions.SystemInitializeOptions = nullptr;
	InitializeOptions.OverrideThreadAffinity = nullptr;

	// Log initialization parameters
	UE_LOG(LogEpicAdapter, Log, TEXT("EOS_InitializeOptions:"));
	UE_LOG(LogEpicAdapter, Log, TEXT("  ApiVersion: %d"), InitializeOptions.ApiVersion);
	UE_LOG(LogEpicAdapter, Log, TEXT("  ProductName: %s"), ANSI_TO_TCHAR(InitializeOptions.ProductName));
	UE_LOG(LogEpicAdapter, Log, TEXT("  ProductVersion: %s"), ANSI_TO_TCHAR(InitializeOptions.ProductVersion));

	// Attempt EOS initialization
	EOS_EResult InitResult = EOS_Initialize(&InitializeOptions);
	
	if(InitResult == EOS_EResult::EOS_Success)
	{
		UE_LOG(LogEpicAdapter, Warning, TEXT("EOS SDK initialized successfully"));
		return true;
	}
	else if(InitResult == EOS_EResult::EOS_AlreadyConfigured)
	{
		UE_LOG(LogEpicAdapter, Warning, TEXT("EOS SDK already configured"));
		return true;
	}
	else
	{
		UE_LOG(LogEpicAdapter, Error, TEXT("EOS Initialization Failed: %d"), (int32)InitResult);
		return false;
	}
}

bool FEpicAdapterModule::InitializeEOSPlatformInstance()
{
	// EOS Platform Creation
	EOS_Platform_Options PlatformOptions = {};
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	
	// Set Product, Sandbox, and Deployment IDs (same as your OnlineServiceLauncher)
	PlatformOptions.ProductId = "e53c70a40f974362a7de76915468b2ad";
	PlatformOptions.SandboxId = "c0056b26231346dfb13cc35055ac05e6";      
	PlatformOptions.DeploymentId = "73e1290dd40043c8817af3cfdd068d2d";     
	
	// Set client credentials
	PlatformOptions.ClientCredentials.ClientId = "xyza7891KjugKpLoeWAUxHxIzf07xQxP";
	PlatformOptions.ClientCredentials.ClientSecret = "ZHUjAs4theNwf3Txez7liV9+1ew4qWR1yW8PNYMsNg4";
	
	// Cache Directory
	FString FinalCacheDirectory = GetCacheDirectory();
	UE_LOG(LogEpicAdapter, Log, TEXT("Using Cache Directory for EOS: %s"), *FinalCacheDirectory);
	static std::string PersistentCacheDir = TCHAR_TO_UTF8(*FinalCacheDirectory);
	PlatformOptions.CacheDirectory = PersistentCacheDir.c_str();
	
	// Platform flags
	PlatformOptions.Flags = EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D9 |
							EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D10 |
							EOS_PF_WINDOWS_ENABLE_OVERLAY_OPENGL;
	
	// Only add LOADING_IN_EDITOR flag when actually in editor
	#if WITH_EDITOR
	PlatformOptions.Flags |= EOS_PF_LOADING_IN_EDITOR;
	#endif

	PlatformOptions.bIsServer = false;
	PlatformOptions.EncryptionKey = "1111111111111111111111111111111111111111111111111111111111111111";
	PlatformOptions.TickBudgetInMilliseconds = 0;

	// Enable RTC features
	EOS_Platform_RTCOptions RtcOptions = {};
	RtcOptions.ApiVersion = EOS_PLATFORM_RTCOPTIONS_API_LATEST;
	RtcOptions.BackgroundMode = EOS_ERTCBackgroundMode::EOS_RTCBM_LeaveRooms;

	// Platform-specific RTC options
	#if PLATFORM_WINDOWS
	// Windows-specific RTC options
	EOS_Windows_RTCOptions WindowsRtcOptions = {};
	WindowsRtcOptions.ApiVersion = EOS_WINDOWS_RTCOPTIONS_API_LATEST;

	// XAudio2 DLL path setup
	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString XAudio2DllPath;
  	
  	#if PLATFORM_64BITS
		XAudio2DllPath = FPaths::Combine(ProjectDir, TEXT("ThirdParty/EOS-SDK-47370208-Release-v1.18.1.2/SDK/Bin/x64/xaudio2_9redist.dll"));
  	#else
		XAudio2DllPath = FPaths::Combine(ProjectDir, TEXT("ThirdParty/EOS-SDK-47370208-Release-v1.18.1.2/SDK/Bin/x86/xaudio2_9redist.dll"));
  	#endif

	// Check if file exists, try alternative path if not
	if (!FPaths::FileExists(XAudio2DllPath))
	{
		FString AlternativePath = FPaths::Combine(ProjectDir, TEXT("ThirdParty/XAudio/xaudio2_9redist.dll"));
		if (FPaths::FileExists(AlternativePath))
		{
			XAudio2DllPath = AlternativePath;
		}
		else
		{
			UE_LOG(LogEpicAdapter, Warning, TEXT("XAudio2 DLL not found - RTC features may not work correctly"));
		}
	}

	std::string XAudio29DllPath = TCHAR_TO_UTF8(*XAudio2DllPath);
	WindowsRtcOptions.XAudio29DllPath = XAudio29DllPath.c_str();
	RtcOptions.PlatformSpecificOptions = &WindowsRtcOptions;

	#elif PLATFORM_MAC
	// macOS-specific RTC options
	EOS_Mac_RTCOptions MacRtcOptions = {};
	MacRtcOptions.ApiVersion = EOS_MAC_RTCOPTIONS_API_LATEST;
	RtcOptions.PlatformSpecificOptions = &MacRtcOptions;

	#elif PLATFORM_LINUX
	// Linux-specific RTC options
	EOS_Linux_RTCOptions LinuxRtcOptions = {};
	LinuxRtcOptions.ApiVersion = EOS_LINUX_RTCOPTIONS_API_LATEST;
	RtcOptions.PlatformSpecificOptions = &LinuxRtcOptions;

	#else
	RtcOptions.PlatformSpecificOptions = nullptr;
	UE_LOG(LogEpicAdapter, Warning, TEXT("Platform-specific RTC options not configured for this platform"));
	#endif

	PlatformOptions.RTCOptions = &RtcOptions;

	// Handle integrated platform options
	if (!PlatformOptions.IntegratedPlatformOptionsContainerHandle)
	{
		EOS_IntegratedPlatform_CreateIntegratedPlatformOptionsContainerOptions CreateOptions = {};
		CreateOptions.ApiVersion = EOS_INTEGRATEDPLATFORM_CREATEINTEGRATEDPLATFORMOPTIONSCONTAINER_API_LATEST;
		EOS_EResult Result = EOS_IntegratedPlatform_CreateIntegratedPlatformOptionsContainer(&CreateOptions, &PlatformOptions.IntegratedPlatformOptionsContainerHandle);
		if (Result != EOS_EResult::EOS_Success)
		{
			UE_LOG(LogEpicAdapter, Error, TEXT("Failed to create integrated platform options container"));
		}
	}

	#if ALLOW_RESERVED_OPTIONS
	// SetReservedPlatformOptions(PlatformOptions); // You'd need to implement this if needed
	#else
	PlatformOptions.Reserved = nullptr;
	#endif

	// Log platform options
	UE_LOG(LogEpicAdapter, Log, TEXT("EOS_Platform_Options:"));
	UE_LOG(LogEpicAdapter, Log, TEXT("  ProductId: %s"), ANSI_TO_TCHAR(PlatformOptions.ProductId));
	UE_LOG(LogEpicAdapter, Log, TEXT("  SandboxId: %s"), ANSI_TO_TCHAR(PlatformOptions.SandboxId));
	UE_LOG(LogEpicAdapter, Log, TEXT("  DeploymentId: %s"), ANSI_TO_TCHAR(PlatformOptions.DeploymentId));
	UE_LOG(LogEpicAdapter, Log, TEXT("  bIsServer: %d"), PlatformOptions.bIsServer);

	// Create the EOS Platform instance
	PlatformHandle = EOS_Platform_Create(&PlatformOptions);
	
	// Release the integrated platform options container
	if (PlatformOptions.IntegratedPlatformOptionsContainerHandle)
	{
		EOS_IntegratedPlatformOptionsContainer_Release(PlatformOptions.IntegratedPlatformOptionsContainerHandle);
	}

	if (PlatformHandle == nullptr)
	{
		UE_LOG(LogEpicAdapter, Error, TEXT("EOS Platform creation failed"));
		return false;
	}
	
	UE_LOG(LogEpicAdapter, Warning, TEXT("EOS Platform successfully created"));
	return true;
}

FString FEpicAdapterModule::GetCacheDirectory() const
{
	// Determine base folder
	FString BaseFolder = bUseAppDataFolder 
		? FPlatformMisc::GetEnvironmentVariable(TEXT("APPDATA")) 
		: FPaths::ProjectSavedDir();
		
	// Fall back to ProjectSavedDir if APPDATA is empty
	if (bUseAppDataFolder && BaseFolder.IsEmpty())
	{
		UE_LOG(LogEpicAdapter, Warning, TEXT("APPDATA environment variable is empty! Falling back to ProjectSavedDir"));
		BaseFolder = FPaths::ProjectSavedDir();
	}
	
	// Convert to full path
	BaseFolder = FPaths::ConvertRelativePathToFull(BaseFolder);
	
	// Build cache path
	FString CachePath = BaseFolder;
	for (const FString& SubDir : CacheSubDirs)
	{
		CachePath = FPaths::Combine(CachePath, SubDir);
	}
	
	CachePath = FPaths::ConvertRelativePathToFull(CachePath);
	UE_LOG(LogEpicAdapter, Log, TEXT("Resolved Cache Directory: %s"), *CachePath);
	
	// Ensure directory exists
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CachePath) && !PlatformFile.CreateDirectoryTree(*CachePath))
	{
		UE_LOG(LogEpicAdapter, Error, TEXT("Failed to create cache directory: %s"), *CachePath);
	}
	
	return CachePath;
}

void FEpicAdapterModule::ConfigureEOSLogger()
{
	// Set logging callback
	EOS_Logging_SetCallback(&FEpicAdapterModule::ProcessLogConfiguration);

	// Set log levels for various categories
	EOS_Logging_SetLogLevel((EOS_ELogCategory)0,  (EOS_ELogLevel)500); // Core
	EOS_Logging_SetLogLevel((EOS_ELogCategory)1,  (EOS_ELogLevel)500); // Auth
	EOS_Logging_SetLogLevel((EOS_ELogCategory)13, (EOS_ELogLevel)500); // Connect
	EOS_Logging_SetLogLevel((EOS_ELogCategory)18, (EOS_ELogLevel)500); // Lobby
	EOS_Logging_SetLogLevel((EOS_ELogCategory)8,  (EOS_ELogLevel)500); // Sessions
	EOS_Logging_SetLogLevel((EOS_ELogCategory)2,  (EOS_ELogLevel)500); // Friends
	EOS_Logging_SetLogLevel((EOS_ELogCategory)4,  (EOS_ELogLevel)500); // UserInfo

	UE_LOG(LogEpicAdapter, Log, TEXT("EOS logging system configured"));
}

void FEpicAdapterModule::ProcessLogConfiguration(const EOS_LogMessage* Message)
{
	if (Message)
	{
		FString Category = UTF8_TO_TCHAR(Message->Category);
		FString LogMessage = UTF8_TO_TCHAR(Message->Message);

		switch ((int)Message->Level)
		{
			case 100: // EOS_LOG_Fatal
				UE_LOG(LogEpicAdapter, Fatal, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
				break;
			case 200: // EOS_LOG_Error
				UE_LOG(LogEpicAdapter, Error, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
				break;
			case 300: // EOS_LOG_Warning
				UE_LOG(LogEpicAdapter, Warning, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
				break;
			case 400: // EOS_LOG_Info
				UE_LOG(LogEpicAdapter, Log, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
				break;
			case 500: // EOS_LOG_Verbose
				UE_LOG(LogEpicAdapter, Verbose, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
				break;
			case 600: // EOS_LOG_VeryVerbose
				UE_LOG(LogEpicAdapter, VeryVerbose, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
				break;
			default:
				UE_LOG(LogEpicAdapter, Log, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
				break;
		}
	}
}

void FEpicAdapterModule::ShutdownEOSService()
{
	#if WITH_EDITOR
	UE_LOG(LogEpicAdapter, Log, TEXT("Editor mode: Skipping EOS shutdown to preserve platform handle"));
	#else
	// Release the platform handle
	if (PlatformHandle != nullptr)
	{
		EOS_Platform_Release(PlatformHandle);
		PlatformHandle = nullptr;
		UE_LOG(LogEpicAdapter, Log, TEXT("EOS Platform released"));
	}

	// Shutdown the EOS SDK
	EOS_Shutdown();
	UE_LOG(LogEpicAdapter, Log, TEXT("EOS SDK shutdown"));
	#endif
}

#endif // WITH_EOS_SDK

// Module implementation
IMPLEMENT_MODULE(FEpicAdapterModule, EpicAdapter)