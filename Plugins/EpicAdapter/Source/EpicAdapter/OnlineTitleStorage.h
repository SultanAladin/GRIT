// OnlineTitleStorage.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineServiceLauncher.h"
#include "OnlineSessionAuthenticator.h"
#include "eos_titlestorage.h"
#include "eos_titlestorage_types.h"

#include "OnlineTitleStorage.generated.h"

/** Structure for file metadata */
USTRUCT(BlueprintType)
struct FTitleStorageFileInfo
{
    GENERATED_BODY()

    /** File name */
    UPROPERTY(BlueprintReadOnly, Category = "Online|TitleStorage")
    FString FileName;

    /** File size in bytes */
    UPROPERTY(BlueprintReadOnly, Category = "Online|TitleStorage")
    int32 FileSizeBytes = 0;

    /** MD5 hash of the file */
    UPROPERTY(BlueprintReadOnly, Category = "Online|TitleStorage")
    FString MD5Hash;

    /** Unencrypted data size */
    UPROPERTY(BlueprintReadOnly, Category = "Online|TitleStorage")
    int32 UnencryptedDataSizeBytes = 0;
};

/** Delegate for when file list query completes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFileListQueried, bool, bSuccess, const TArray<FString>&, FileNames);

/** Delegate for when file download completes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFileDownloaded, bool, bSuccess, const FString&, FileName, const FString&, FileContent);

/** Delegate for download progress updates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDownloadProgress, const FString&, FileName, float, Progress, int32, BytesTransferred);

/**
 * OnlineTitleStorage - Game Instance Subsystem
 * 
 * Handles downloading and caching of title storage files using Epic Online Services (EOS).
 * 
 * WHAT IS TITLE STORAGE?
 * Title Storage is a cloud-based file storage system provided by Epic Games Services that allows
 * developers to store and distribute read-only game data files to players. These files are managed
 * by the game developer and can be updated without requiring a game patch.
 * 
 * COMMON USE CASES:
 * 
 * 1. GAME CONFIGURATION FILES:
 *    - Server lists and connection info
 *    - Game balance parameters (weapon damage, character stats)
 *    - Feature flags and A/B testing configurations
 *    - Seasonal event configurations
 * 
 * 2. DYNAMIC CONTENT:
 *    - Daily/weekly challenges and objectives
 *    - News feeds and announcements
 *    - Store promotions and special offers
 *    - Tournament brackets and schedules
 * 
 * 3. LOCALIZATION DATA:
 *    - Translation files for different languages
 *    - Region-specific content variations
 * 
 * 4. GAME DATA UPDATES:
 *    - Map rotations and playlist updates
 *    - New item definitions and properties
 *    - Updated quest chains or storylines
 * 
 * EXAMPLE FILE STRUCTURE:
 * - "config/server_list.json" - List of game servers by region
 * - "events/halloween_2024.json" - Halloween event configuration
 * - "balance/weapon_stats.csv" - Current weapon damage values
 * - "news/latest_updates.json" - In-game news feed content
 * - "localization/strings_es.json" - Spanish language strings
 * 
 * BENEFITS OVER TRADITIONAL PATCHES:
 * - Instant updates without client downloads
 * - A/B testing capabilities with user segmentation
 * - Regional content variations
 * - Reduced patch sizes and frequency
 * - Real-time content management
 * 
 * FILES ARE READ-ONLY: Players can download and cache these files but cannot modify them.
 * All file management is done through the Epic Games Developer Portal.
 */
UCLASS()
class EPICADAPTER_API UOnlineTitleStorage : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Constructor */
    UOnlineTitleStorage();

    /** USubsystem interface */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    //
    // Public API Functions
    //

    /** Query the list of available files with specific tags filter */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage")
    bool QueryFileList(const TArray<FString>& Tags);

    /** Query all available files (no tag filter) */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage", CallInEditor, DisplayName = "Query All Files")
    bool QueryAllFiles();

    /** Download a specific file by name */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage")
    bool DownloadFile(const FString& FileName);

    /** Get locally cached file content (returns empty string if not available) */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage")
    FString GetCachedFileContent(const FString& FileName, bool& bIsAvailable);

    /** Get file metadata for a specific file */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage")
    FTitleStorageFileInfo GetFileInfo(const FString& FileName);

    /** Get list of all currently cached file names */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage")
    TArray<FString> GetCachedFileNames() const;

    /** Cancel current file transfer if any */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage")
    void CancelCurrentTransfer();

    /** Clear all cached file data */
    UFUNCTION(BlueprintCallable, Category = "Online|TitleStorage")
    void ClearCache();

    //
    // Delegates
    //

    /** Fired when file list query completes */
    UPROPERTY(BlueprintAssignable, Category = "Online|TitleStorage")
    FOnFileListQueried OnFileListQueriedDelegate;

    /** Fired when file download completes */
    UPROPERTY(BlueprintAssignable, Category = "Online|TitleStorage")
    FOnFileDownloaded OnFileDownloadedDelegate;

    /** Fired when download progress updates */
    UPROPERTY(BlueprintAssignable, Category = "Online|TitleStorage")
    FOnDownloadProgress OnDownloadProgressDelegate;

private:
    /** Initialize EOS handles */
    bool InitializeEOSHandles();

    /** Cleanup current transfer state */
    void CleanupCurrentTransfer();

    /** Internal implementation for querying file list */
    bool QueryFileListInternal(const TArray<FString>& Tags);

    //
    // Static EOS Callbacks
    //

    /** Callback for file list query completion */
    static void EOS_CALL OnFileListRetrievedCallback(const EOS_TitleStorage_QueryFileListCallbackInfo* Data);

    /** Callback for file data chunks during download */
    static EOS_TitleStorage_EReadResult EOS_CALL OnFileDataReceivedCallback(const EOS_TitleStorage_ReadFileDataCallbackInfo* Data);

    /** Callback for file download completion */
    static void EOS_CALL OnFileDownloadCompleteCallback(const EOS_TitleStorage_ReadFileCallbackInfo* Data);

    /** Callback for download progress updates */
    static void EOS_CALL OnFileTransferProgressCallback(const EOS_TitleStorage_FileTransferProgressCallbackInfo* Data);

    //
    // Member Variables
    //

    /** Reference to the service launcher */
    UPROPERTY()
    UOnlineServiceLauncher* ServiceLauncher;

    /** Reference to the session authenticator */
    UPROPERTY()
    UOnlineSessionAuthenticator* SessionAuthenticator;

    /** EOS Title Storage interface handle */
    EOS_HTitleStorage TitleStorageHandle;

    /** Current file transfer request handle */
    EOS_HTitleStorageFileTransferRequest CurrentTransferHandle;

    /** Current file being downloaded */
    FString CurrentDownloadFileName;

    /** Download progress (0.0 to 1.0) */
    float CurrentDownloadProgress;

    /** Cached file contents - FileName -> Content */
    UPROPERTY()
    TMap<FString, FString> CachedFiles;

    /** File metadata cache */
    TMap<FString, FTitleStorageFileInfo> FileMetadataCache;

    /** Temporary data for current download */
    TArray<uint8> CurrentDownloadData;

    /** Total expected size of current download */
    int32 CurrentDownloadTotalSize;

    /** Bytes received so far for current download */
    int32 CurrentDownloadBytesReceived;
};