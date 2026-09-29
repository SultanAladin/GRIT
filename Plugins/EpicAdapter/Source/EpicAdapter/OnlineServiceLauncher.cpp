#include "OnlineServiceLauncher.h"
#include "EpicAdapter.h" // Include to access the pre-initialized platform handle
#include "HAL/PlatformFilemanager.h"
#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogOnlineServiceLauncher, Log, All);

UOnlineServiceLauncher::UOnlineServiceLauncher()
{
    PlatformHandle = nullptr;
}

void UOnlineServiceLauncher::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Get the pre-initialized platform handle from EpicAdapter
    PlatformHandle = FEpicAdapterModule::GetPlatformHandle();
    
    if (PlatformHandle != nullptr)
    {
        UE_LOG(LogOnlineServiceLauncher, Warning, TEXT("Successfully obtained pre-initialized EOS Platform handle"));
        
        // Configure logging if enabled
        if (EnableLogStream)
        {
            ConfigureLogger();
        }
    }
    else
    {
        UE_LOG(LogOnlineServiceLauncher, Error, TEXT("Failed to obtain EOS Platform handle from EpicAdapter"));
    }
}

void UOnlineServiceLauncher::ConfigureLogger()
{
    // Set logging callback
    EOS_Logging_SetCallback(&UOnlineServiceLauncher::ProcessLogConfiguration);

    // Use integer values for log categories and levels
    EOS_Logging_SetLogLevel((EOS_ELogCategory)0,  (EOS_ELogLevel)500); // Core
    EOS_Logging_SetLogLevel((EOS_ELogCategory)1,  (EOS_ELogLevel)500); // Auth
    EOS_Logging_SetLogLevel((EOS_ELogCategory)13, (EOS_ELogLevel)500); // Connect
    EOS_Logging_SetLogLevel((EOS_ELogCategory)18, (EOS_ELogLevel)500); // Lobby
    EOS_Logging_SetLogLevel((EOS_ELogCategory)8,  (EOS_ELogLevel)500); // Sessions
    EOS_Logging_SetLogLevel((EOS_ELogCategory)2,  (EOS_ELogLevel)500); // Friends
    EOS_Logging_SetLogLevel((EOS_ELogCategory)4,  (EOS_ELogLevel)500); // UserInfo

    UE_LOG(LogOnlineServiceLauncher, Log, TEXT("EOS logging system configured."));
}

void UOnlineServiceLauncher::ProcessLogConfiguration(const EOS_LogMessage* Message)
{
    if (Message)
    {
        FString Category = UTF8_TO_TCHAR(Message->Category);
        FString LogMessage = UTF8_TO_TCHAR(Message->Message);

        switch ((int)Message->Level)
        {
            case 100: // EOS_LOG_Fatal
                UE_LOG(LogOnlineServiceLauncher, Fatal, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
                break;
            case 200: // EOS_LOG_Error
                UE_LOG(LogOnlineServiceLauncher, Error, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
                break;
            case 300: // EOS_LOG_Warning
                UE_LOG(LogOnlineServiceLauncher, Warning, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
                break;
            case 400: // EOS_LOG_Info
                UE_LOG(LogOnlineServiceLauncher, Log, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
                break;
            case 500: // EOS_LOG_Verbose
                UE_LOG(LogOnlineServiceLauncher, Verbose, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
                break;
            case 600: // EOS_LOG_VeryVerbose
                UE_LOG(LogOnlineServiceLauncher, VeryVerbose, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
                break;
            default:
                UE_LOG(LogOnlineServiceLauncher, Log, TEXT("[EOS] [%s] %s"), *Category, *LogMessage);
                break;
        }
    }
}

void UOnlineServiceLauncher::Tick(float DeltaTime)
{
    // Tick the EOS Platform if it's valid
    if (PlatformHandle != nullptr)
    {
        EOS_Platform_Tick(PlatformHandle);
    }
}

bool UOnlineServiceLauncher::IsTickable() const
{
    // Only tick if we have a valid platform handle
    return PlatformHandle != nullptr;
}

TStatId UOnlineServiceLauncher::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UOnlineServiceLauncher, STATGROUP_Tickables);
}

// Keep these functions for compatibility with existing code that might call them
bool UOnlineServiceLauncher::InitializeService()
{
    UE_LOG(LogOnlineServiceLauncher, Warning, TEXT("EOS Service already initialized by EpicAdapter module"));
    return (PlatformHandle != nullptr);
}

bool UOnlineServiceLauncher::InitializePlatformInstance()
{
    UE_LOG(LogOnlineServiceLauncher, Warning, TEXT("EOS Platform already initialized by EpicAdapter module"));
    return (PlatformHandle != nullptr);
}

FString UOnlineServiceLauncher::GetCacheDirectory() const
{
    // Determine base folder based on the bUseAppDataFolder flag
    FString BaseFolder = bUseAppDataFolder 
        ? FPlatformMisc::GetEnvironmentVariable(TEXT("APPDATA")) 
        : FPaths::ProjectSavedDir();
        
    // Fall back to ProjectSavedDir if APPDATA is empty
    if (bUseAppDataFolder && BaseFolder.IsEmpty())
    {
        UE_LOG(LogOnlineServiceLauncher, Warning, TEXT("APPDATA environment variable is empty! Falling back to ProjectSavedDir."));
        BaseFolder = FPaths::ProjectSavedDir();
    }
    
    // Convert to full path
    BaseFolder = FPaths::ConvertRelativePathToFull(BaseFolder);
    
    // Build cache path by combining base folder with all subdirectories
    FString CachePath = BaseFolder;
    for (const FString& SubDir : CacheSubDirs)
    {
        CachePath = FPaths::Combine(CachePath, SubDir);
    }
    
    CachePath = FPaths::ConvertRelativePathToFull(CachePath);
    
    return CachePath;
}

EOS_HPlatform UOnlineServiceLauncher::GetPlatformHandle()
{
    if (PlatformHandle == nullptr)
    {
        // Try to get the handle from EpicAdapter if we don't have it
        PlatformHandle = FEpicAdapterModule::GetPlatformHandle();
        if (PlatformHandle == nullptr)
        {
            UE_LOG(LogOnlineServiceLauncher, Error, TEXT("EOS Platform handle is null - EpicAdapter may not have initialized properly"));
        }
    }
    
    return PlatformHandle;
}

void UOnlineServiceLauncher::ShutdownService()
{
    // Don't shutdown EOS here since it's managed by EpicAdapter
    // Just clear our local handle reference
    PlatformHandle = nullptr;
    UE_LOG(LogOnlineServiceLauncher, Log, TEXT("OnlineServiceLauncher shutdown - EOS Platform managed by EpicAdapter"));
}

void UOnlineServiceLauncher::Deinitialize()
{
    ShutdownService();
    Super::Deinitialize();
}