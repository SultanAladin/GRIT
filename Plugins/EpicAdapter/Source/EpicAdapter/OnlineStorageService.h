// OnlineStorageService.h
/*
 * UOnlineStorageService - Cloud Storage Management Subsystem
 * 
 * PURPOSE:
 * This subsystem provides seamless cloud storage functionality using Epic Online Services (EOS) PlayerDataStorage.
 * It handles file uploads, downloads, queries, and deletions with automatic local caching and progress tracking.
 * 
 * WHY USE THIS:
 * - Automatic cloud backup of user data across devices
 * - Seamless data synchronization between game sessions
 * - Persistent storage that survives game uninstalls
 * - Built-in progress tracking and error handling
 * - Local caching for offline access
 * 
 * TYPICAL USE CASES:
 * - Save game data: player progress, achievements, unlocks
 * - User preferences: graphics settings, key bindings, audio levels
 * - Vehicle/character profiles: customizations, loadouts, stats
 * - Session data: lobby preferences, matchmaking settings
 * - Performance logs: crash reports, telemetry data
 * 
 * EXAMPLE USAGE:
 * 
 * // Get the service
 * UOnlineStorageService* StorageService = GetGameInstance()->GetSubsystem<UOnlineStorageService>();
 * 
 * // Upload a save file
 * FString SavePath = StorageService->GetStoragePath(EStorageCategory::UserState) + "/SaveGame.json";
 * StorageService->UploadFile(PlayerUserId, "SaveGame.json", SavePath, 4096, 
 *     FOnFileTransferProgressDelegate::CreateUFunction(this, FName("OnUploadProgress")),
 *     FOnFileTransferCompleteDelegate::CreateUFunction(this, FName("OnUploadComplete")));
 * 
 * // Download user settings
 * StorageService->DownloadFile("UserSettings.cfg",
 *     FOnFileTransferProgressDelegate::CreateUFunction(this, FName("OnDownloadProgress")),
 *     FOnFileTransferCompleteDelegate::CreateUFunction(this, FName("OnDownloadComplete")));
 * 
 * // Query all user files
 * StorageService->QueryFileList(UserIdString,
 *     FOnFileListQueryCompleteDelegate::CreateUFunction(this, FName("OnFileListReceived")));
 */

#pragma once

#include "CoreMinimal.h"
#include "eos_playerdatastorage.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSessionAuthenticator.h"
#include "OnlineServiceLauncher.h"
#include "Engine/World.h"
#include "OnlineStorageService.generated.h"

// Forward declarations
struct FEOSFileTransferContext;
struct FFileDeleteContext;
struct FFileListQueryContext;

/* Storage directory categories for organized file management */
UENUM(BlueprintType)
enum class EStorageCategory : uint8
{
    UserPreferences      UMETA(DisplayName = "User Preferences"),
    AppSettings          UMETA(DisplayName = "App Settings"),
    UserConfiguration    UMETA(DisplayName = "User Configuration"),
    UserState            UMETA(DisplayName = "User State"),
    Lobbies              UMETA(DisplayName = "Lobbies"),
    Sessions             UMETA(DisplayName = "Sessions"),
    CrashReports         UMETA(DisplayName = "Crash Reports"),
    PerformanceLogs      UMETA(DisplayName = "Performance Logs"),
    InputSettings        UMETA(DisplayName = "Input Settings"),
    VehicleProfiles      UMETA(DisplayName = "Vehicle Profiles")
};

/* File information structure for metadata tracking */
/* File information structure for metadata tracking */
USTRUCT(BlueprintType)
struct FCloudFileInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString FileName;

    UPROPERTY(BlueprintReadOnly)
    int32 FileSizeBytes;  // Changed from uint32 to int32

    UPROPERTY(BlueprintReadOnly)
    FString MD5Hash;

    UPROPERTY(BlueprintReadOnly)
    FDateTime LastModified;

    FCloudFileInfo()
        : FileSizeBytes(0)
        , LastModified(0)
    {}
};

// Delegates for file transfer operations
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnFileTransferProgressDelegate, float, ProgressPercentage);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnFileTransferCompleteDelegate, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnFileListQueryCompleteDelegate, bool, bSuccess, const TArray<FCloudFileInfo>&, FileList);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnFileDeleteCompleteDelegate, bool, bSuccess, const FString&, FileName);

/* Log category for the Online Storage Service */
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineStorage, Log, All);

/* 
 * Subsystem for managing online storage operations using EOS PlayerDataStorage 
 * Provides cloud file management with local caching and progress tracking
 */
UCLASS(BlueprintType, Blueprintable)
class EPICADAPTER_API UOnlineStorageService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UOnlineStorageService();
    virtual ~UOnlineStorageService();
    
    // Begin USubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // End USubsystem
    
    /* Initializes the EOS Player Data Storage interface, returns success status */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    bool InitializeOnlineStorage();

    /* Check if the storage service is properly initialized and ready to use */
    UFUNCTION(BlueprintPure, Category = "Online|Storage")
    bool IsStorageInitialized() const;
    
    /* Base subdirectories for local storage (default creates [BaseFolder]/GTX/LocalArchive) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|EOS|Storage")
    TArray<FString> LocalArchiveBaseDirs = { TEXT("GTX"), TEXT("LocalArchive") };
    
    /* Gets the full path for a specific storage category, creating directory if requested */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    FString GetStoragePath(EStorageCategory Category, bool bCreateIfMissing = true);
    
    /* Gets full path for custom storage directory, creating if requested */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    FString GetCustomStoragePath(const TArray<FString>& CustomPathArray, bool bCreateIfMissing = true);

    /* Blueprint-friendly upload function using authenticated user */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage", meta = (CallInEditor = "true"))
    bool UploadFileFromPath(const FString& FileName, const FString& FilePath, 
        const FOnFileTransferProgressDelegate& OnProgress, const FOnFileTransferCompleteDelegate& OnComplete);

    /* Blueprint-friendly upload function with data array */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    bool UploadFileFromData(const FString& FileName, const TArray<uint8>& FileData,
        const FOnFileTransferProgressDelegate& OnProgress, const FOnFileTransferCompleteDelegate& OnComplete);
        
    /* Core upload function with explicit user ID */
    bool UploadFile(EOS_ProductUserId LocalUserId, const FString& FileName, const FString& FilePath, 
        uint32 ChunkSizeBytes, FOnFileTransferProgressDelegate OnProgress, FOnFileTransferCompleteDelegate OnComplete);

    /* Downloads a file from cloud storage with progress and completion callbacks */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    bool DownloadFile(const FString& FileName, const FOnFileTransferProgressDelegate& OnProgress, 
        const FOnFileTransferCompleteDelegate& OnComplete);

    /* Deletes a file from cloud storage using authenticated user */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    bool DeleteFile(const FString& FileName, const FOnFileDeleteCompleteDelegate& OnDeleteCompleteDelegate);

    /* Queries list of files in cloud storage for authenticated user */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    bool QueryFileList(const FOnFileListQueryCompleteDelegate& OnQueryCompleteDelegate);

    /* Cancels an active file transfer by name, returns true if found and canceled */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    bool CancelFileTransfer(const FString& FileName);

    /* Gets list of currently active file transfers */
    UFUNCTION(BlueprintPure, Category = "Online|Storage")
    TArray<FString> GetActiveTransfers() const;

    /* Checks if a specific file is currently being transferred */
    UFUNCTION(BlueprintPure, Category = "Online|Storage")
    bool IsFileBeingTransferred(const FString& FileName) const;

    /* Clears local cache directory */
    UFUNCTION(BlueprintCallable, Category = "Online|Storage")
    void ClearLocalCache();

protected:
    /* Process completed file download data */
    void ProcessCompletedDownload(const FString& FileName, const TArray<uint8>& FileData, bool bSuccess);

    /* Removes file from local cache and active transfers */
    void RemoveFromLocalCache(const FString& FileName);

    /* Gets the authenticated user's ProductUserId */
    EOS_ProductUserId GetAuthenticatedUserId() const;

    /* Validates that the service is ready for operations */
    bool ValidateServiceReady() const;
    
private:
    /* Handle to the EOS Player Data Storage interface */
    EOS_HPlayerDataStorage PlayerDataStorageHandle;
    
    /* Session authenticator for validating player credentials */
    UPROPERTY()
    TObjectPtr<UOnlineSessionAuthenticator> SessionAuthenticator;
    
    /* Reference to the Online Service Launcher subsystem */
    UPROPERTY()
    TObjectPtr<UOnlineServiceLauncher> ServiceLauncher;
    
    /* Map of active file transfers */
    TMap<FString, EOS_HPlayerDataStorageFileTransferRequest> ActiveTransfers;
    
    /* Path to local cache directory */
    FString LocalCacheDirectory;

    /* Map of storage directories for various data types */
    TMap<FString, TArray<FString>> StorageDirectories;

    /* Sets up default storage directories */
    void InitializeStorageDirectories();

    /* Stores downloaded file to local cache */
    void CacheDownloadedFile(const FString& FileName, const TArray<uint8>& FileData);

    /* Internal upload function with data array */
    bool UploadFileInternal(EOS_ProductUserId LocalUserId, const FString& FileName, const TArray<uint8>& FileData,
        uint32 ChunkSizeBytes, const FOnFileTransferProgressDelegate& OnProgress, const FOnFileTransferCompleteDelegate& OnComplete);

    // Static EOS Callbacks
    static EOS_PlayerDataStorage_EWriteResult EOS_CALL OnWriteFileDataCallback(
        const EOS_PlayerDataStorage_WriteFileDataCallbackInfo* Data, void* OutDataBuffer, uint32_t* OutDataWritten);
        
    static void EOS_CALL OnFileTransferProgressCallback(
        const EOS_PlayerDataStorage_FileTransferProgressCallbackInfo* Data);
        
    static void EOS_CALL OnWriteFileCompleteCallback(
        const EOS_PlayerDataStorage_WriteFileCallbackInfo* Data);

    static EOS_PlayerDataStorage_EReadResult EOS_CALL OnReadFileDataCallback(
        const EOS_PlayerDataStorage_ReadFileDataCallbackInfo* Data);

    static void EOS_CALL OnReadFileCompleteCallback(
        const EOS_PlayerDataStorage_ReadFileCallbackInfo* Data);
    
    static void EOS_CALL OnDeleteFileCompleteCallback(
        const EOS_PlayerDataStorage_DeleteFileCallbackInfo* Data);

    static void EOS_CALL OnQueryFileListCompleteCallback(
        const EOS_PlayerDataStorage_QueryFileListCallbackInfo* Data);
};

/* Structure to track file deletion operations */
struct FFileDeleteContext
{
    void* UserPointer;
    FString FileName;
    FOnFileDeleteCompleteDelegate OnComplete;
    
    FFileDeleteContext() : UserPointer(nullptr) {}
};

/* Structure to hold file list query context data */
struct FFileListQueryContext
{
    void* UserPointer;
    FOnFileListQueryCompleteDelegate OnComplete;
    
    FFileListQueryContext() : UserPointer(nullptr) {}
};

/* Structure to hold file transfer context data */
struct FEOSFileTransferContext
{
    TArray<uint8> FileData;
    uint32 CurrentPosition;
    FString FileName;
    FOnFileTransferProgressDelegate OnProgress;
    FOnFileTransferCompleteDelegate OnComplete;
    void* UserPointer;
    
    FEOSFileTransferContext() : CurrentPosition(0), UserPointer(nullptr) {}
};