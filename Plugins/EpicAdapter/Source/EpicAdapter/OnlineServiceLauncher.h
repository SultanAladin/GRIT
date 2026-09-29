#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "eos_sdk.h"
#include "eos_logging.h"
#include "eos_rtc_types.h"
#include <string>

// Platform-specific EOS includes
#ifdef _WIN32
#include "Windows/eos_windows.h"
#endif

#include "OnlineServiceLauncher.generated.h"

/**
 * Online service subsystem that handles EOS SDK initialization and management
 */


UCLASS()
class EPICADAPTER_API UOnlineServiceLauncher : public UEngineSubsystem , public FTickableGameObject
{
    GENERATED_BODY()
public:
    /** Default constructor */
    UOnlineServiceLauncher();
    
    /** Initialize the subsystem */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    /** Deinitialize the subsystem */
    virtual void Deinitialize() override;

    //~ Begin EOS Configuration
    
    /** Whether to use APPDATA folder as base directory; falls back to ProjectSavedDir if false */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|EOS|Configuration")
    bool bUseAppDataFolder = true;
    
    /** Subdirectories appended to base folder (default creates [BaseFolder]/GTX/EOSCache) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|EOS|Configuration")
    TArray<FString> CacheSubDirs = { TEXT("GTX"), TEXT("EOSCache") };
    
    /** Flag to control whether to enable EOS log streaming */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|EOS|Logging")
    bool EnableLogStream = true;
    
    //~ End EOS Configuration

     //~ Begin FTickableGameObject Interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    //~ End FTickableGameObject Interface
    
    //~ Begin EOS Functions
    
    /** Returns and creates if needed the fully resolved cache directory path */
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Utility")
    FString GetCacheDirectory() const;
    
    /** Configures the EOS logging system */
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Logging")
    void ConfigureLogger(); 
    
    /** Initializes the online service (EOS or any other) */
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Lifecycle")
    bool InitializeService();
    
    /** Initializes the platform instance (EOS SDK) */
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Lifecycle")
    bool InitializePlatformInstance();

    /** Gets the EOS platform handle, initializing it if necessary @return The EOS platform handle or nullptr if initialization failed*/
    EOS_HPlatform GetPlatformHandle();
  
    /** Shuts down the online service */
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Lifecycle")
    void ShutdownService();

    
    
    //~ End EOS Functions
    
private:
    /** Callback function for processing EOS log messages */
    static void ProcessLogConfiguration(const EOS_LogMessage* Message);
    
    /** Handle to the EOS platform instance */
    EOS_HPlatform PlatformHandle;
};

