// OnlineStorageService.cpp
#include "OnlineStorageService.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "eos_common.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Engine/Engine.h"
#include <string>

DEFINE_LOG_CATEGORY(LogOnlineStorage);

// Static callback implementations
EOS_PlayerDataStorage_EWriteResult UOnlineStorageService::OnWriteFileDataCallback(
    const EOS_PlayerDataStorage_WriteFileDataCallbackInfo* Data,
    void* OutDataBuffer, 
    uint32_t* OutDataWritten)
{
    FEOSFileTransferContext* UploadContext = static_cast<FEOSFileTransferContext*>(Data->ClientData);
    if (!UploadContext)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("OnWriteFileDataCallback: Invalid upload context"));
        return EOS_PlayerDataStorage_EWriteResult::EOS_WR_FailRequest;
    }

    if (UploadContext->CurrentPosition >= static_cast<uint32>(UploadContext->FileData.Num()))
    {
        *OutDataWritten = 0;
        return EOS_PlayerDataStorage_EWriteResult::EOS_WR_CompleteRequest;
    }

    uint32_t BytesRemaining = static_cast<uint32_t>(UploadContext->FileData.Num()) - UploadContext->CurrentPosition;
    uint32_t BytesToWrite = FMath::Min(BytesRemaining, Data->DataBufferLengthBytes);

    FMemory::Memcpy(OutDataBuffer, UploadContext->FileData.GetData() + UploadContext->CurrentPosition, BytesToWrite);
    
    *OutDataWritten = BytesToWrite;
    UploadContext->CurrentPosition += BytesToWrite;

    return (UploadContext->CurrentPosition >= static_cast<uint32>(UploadContext->FileData.Num())) 
        ? EOS_PlayerDataStorage_EWriteResult::EOS_WR_CompleteRequest 
        : EOS_PlayerDataStorage_EWriteResult::EOS_WR_ContinueWriting;
}

void UOnlineStorageService::OnFileTransferProgressCallback(
    const EOS_PlayerDataStorage_FileTransferProgressCallbackInfo* Data)
{
    if (!Data) return;

    float ProgressPercentage = 0.0f;
    if (Data->TotalFileSizeBytes > 0)
    {
        ProgressPercentage = (Data->BytesTransferred / static_cast<float>(Data->TotalFileSizeBytes)) * 100.0f;
    }

    FEOSFileTransferContext* Context = static_cast<FEOSFileTransferContext*>(Data->ClientData);
    if (Context && Context->OnProgress.IsBound())
    {
        AsyncTask(ENamedThreads::GameThread, [Context, ProgressPercentage]() {
            Context->OnProgress.Execute(ProgressPercentage);
        });
    }
}

void UOnlineStorageService::OnWriteFileCompleteCallback(
    const EOS_PlayerDataStorage_WriteFileCallbackInfo* Data)
{
    if (!Data) return;

    FEOSFileTransferContext* Context = static_cast<FEOSFileTransferContext*>(Data->ClientData);
    if (!Context) return;

    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    
    AsyncTask(ENamedThreads::GameThread, [Context, bSuccess]() {
        if (Context->OnComplete.IsBound())
        {
            Context->OnComplete.Execute(bSuccess);
        }
        delete Context;
    });
}

EOS_PlayerDataStorage_EReadResult UOnlineStorageService::OnReadFileDataCallback(
    const EOS_PlayerDataStorage_ReadFileDataCallbackInfo* Data)
{
    if (!Data) return EOS_PlayerDataStorage_EReadResult::EOS_RR_FailRequest;
    
    FEOSFileTransferContext* Context = static_cast<FEOSFileTransferContext*>(Data->ClientData);
    if (!Context) return EOS_PlayerDataStorage_EReadResult::EOS_RR_FailRequest;
    
    if (Data->DataChunkLengthBytes > 0 && Data->DataChunk)
    {
        uint32 CurrentIndex = Context->FileData.Num();
        uint32 NewSize = CurrentIndex + Data->DataChunkLengthBytes;
        Context->FileData.SetNumUninitialized(NewSize);
        FMemory::Memcpy(Context->FileData.GetData() + CurrentIndex, Data->DataChunk, Data->DataChunkLengthBytes);
    }
    
    return EOS_PlayerDataStorage_EReadResult::EOS_RR_ContinueReading;
}

void UOnlineStorageService::OnReadFileCompleteCallback(
    const EOS_PlayerDataStorage_ReadFileCallbackInfo* Data)
{
    if (!Data) return;
    
    FEOSFileTransferContext* Context = static_cast<FEOSFileTransferContext*>(Data->ClientData);
    if (!Context) return;
    
    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    
    UOnlineStorageService* Service = static_cast<UOnlineStorageService*>(Context->UserPointer);
    if (Service && bSuccess)
    {
        AsyncTask(ENamedThreads::GameThread, [Service, FileName = Context->FileName, FileData = Context->FileData, bSuccess]() {
            Service->ProcessCompletedDownload(FileName, FileData, bSuccess);
        });
    }
    
    if (Context->OnComplete.IsBound())
    {
        AsyncTask(ENamedThreads::GameThread, [OnComplete = Context->OnComplete, bSuccess]() {
            OnComplete.Execute(bSuccess);
        });
    }
    
    delete Context;
}

void UOnlineStorageService::OnDeleteFileCompleteCallback(
    const EOS_PlayerDataStorage_DeleteFileCallbackInfo* Data)
{
    if (!Data) return;
    
    FFileDeleteContext* Context = static_cast<FFileDeleteContext*>(Data->ClientData);
    if (!Context) return;
    
    if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE) return;
    
    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    UOnlineStorageService* Service = static_cast<UOnlineStorageService*>(Context->UserPointer);
    
    if (Service && bSuccess)
    {
        Service->RemoveFromLocalCache(Context->FileName);
    }
    
    Context->OnComplete.ExecuteIfBound(bSuccess, Context->FileName);
    delete Context;
}

void UOnlineStorageService::OnQueryFileListCompleteCallback(
    const EOS_PlayerDataStorage_QueryFileListCallbackInfo* Data)
{
    if (!Data) return;
    
    FFileListQueryContext* Context = static_cast<FFileListQueryContext*>(Data->ClientData);
    if (!Context) return;
    
    UOnlineStorageService* Service = static_cast<UOnlineStorageService*>(Context->UserPointer);
    if (!Service) 
    {
        delete Context;
        return;
    }
    
    TArray<FCloudFileInfo> FileList;
    if (Data->ResultCode == EOS_EResult::EOS_Success)
    {
        for (uint32_t Index = 0; Index < Data->FileCount; ++Index)
        {
            EOS_PlayerDataStorage_CopyFileMetadataAtIndexOptions MetadataOptions = {};
            MetadataOptions.ApiVersion = EOS_PLAYERDATASTORAGE_COPYFILEMETADATAATINDEX_API_LATEST;
            MetadataOptions.LocalUserId = Data->LocalUserId;
            MetadataOptions.Index = Index;
            
            EOS_PlayerDataStorage_FileMetadata* FileMetadata = nullptr;
            EOS_EResult MetadataResult = EOS_PlayerDataStorage_CopyFileMetadataAtIndex(
                Service->PlayerDataStorageHandle, &MetadataOptions, &FileMetadata);
            
            if (MetadataResult == EOS_EResult::EOS_Success && FileMetadata)
            {
                FCloudFileInfo FileInfo;
                FileInfo.FileName = UTF8_TO_TCHAR(FileMetadata->Filename);
                FileInfo.FileSizeBytes = FileMetadata->FileSizeBytes;
                FileInfo.MD5Hash = FileMetadata->MD5Hash ? UTF8_TO_TCHAR(FileMetadata->MD5Hash) : TEXT("");
                
                if (FileMetadata->LastModifiedTime != EOS_PLAYERDATASTORAGE_TIME_UNDEFINED)
                {
                    FileInfo.LastModified = FDateTime::FromUnixTimestamp(FileMetadata->LastModifiedTime);
                }
                
                FileList.Add(FileInfo);
                EOS_PlayerDataStorage_FileMetadata_Release(FileMetadata);
            }
        }
    }
    
    Context->OnComplete.ExecuteIfBound(Data->ResultCode == EOS_EResult::EOS_Success, FileList);
    delete Context;
}

// Constructor and Destructor
UOnlineStorageService::UOnlineStorageService()
    : PlayerDataStorageHandle(nullptr)
    , SessionAuthenticator(nullptr)
    , ServiceLauncher(nullptr)
{
    LocalCacheDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CloudCache"));
}

UOnlineStorageService::~UOnlineStorageService()
{
    TArray<FString> TransferKeys;
    ActiveTransfers.GetKeys(TransferKeys);
    
    for (const FString& FileName : TransferKeys)
    {
        CancelFileTransfer(FileName);
    }
}

void UOnlineStorageService::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UOnlineSessionAuthenticator>();
    Super::Initialize(Collection);
    
    UE_LOG(LogOnlineStorage, Log, TEXT("Initializing Online Storage Service"));
    InitializeStorageDirectories();
}

void UOnlineStorageService::Deinitialize()
{
    UE_LOG(LogOnlineStorage, Log, TEXT("Deinitializing Online Storage Service"));
    
    TArray<FString> TransferKeys;
    ActiveTransfers.GetKeys(TransferKeys);
    
    for (const FString& FileName : TransferKeys)
    {
        CancelFileTransfer(FileName);
    }
    
    PlayerDataStorageHandle = nullptr;
    SessionAuthenticator = nullptr;
    ServiceLauncher = nullptr;
    
    Super::Deinitialize();
}

bool UOnlineStorageService::InitializeOnlineStorage()
{
    if (!ServiceLauncher)
    {
        ServiceLauncher = GEngine->GetEngineSubsystem<UOnlineServiceLauncher>();
    }
    
    if (!ServiceLauncher)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Failed to get OnlineServiceLauncher"));
        return false;
    }
    
    EOS_HPlatform PlatformHandle = ServiceLauncher->GetPlatformHandle();
    if (!PlatformHandle)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Invalid EOS Platform handle"));
        return false;
    }
    
    PlayerDataStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(PlatformHandle);
    if (!PlayerDataStorageHandle)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Failed to get PlayerDataStorage interface"));
        return false;
    }

    if (!SessionAuthenticator)
    {
        SessionAuthenticator = GetGameInstance()->GetSubsystem<UOnlineSessionAuthenticator>();
    }
    
    UE_LOG(LogOnlineStorage, Log, TEXT("EOS PlayerDataStorage interface initialized"));
    return true;
}

bool UOnlineStorageService::IsStorageInitialized() const
{
    return PlayerDataStorageHandle != nullptr && SessionAuthenticator != nullptr;
}

FString UOnlineStorageService::GetStoragePath(EStorageCategory Category, bool bCreateIfMissing)
{
    FString CategoryKey = StaticEnum<EStorageCategory>()->GetNameStringByValue(static_cast<int64>(Category));
    TArray<FString> PathArray;
    
    if (StorageDirectories.Contains(CategoryKey))
    {
        PathArray = StorageDirectories[CategoryKey];
    }
    else
    {
        PathArray = LocalArchiveBaseDirs;
        PathArray.Add(CategoryKey);
    }
    
    return GetCustomStoragePath(PathArray, bCreateIfMissing);
}

FString UOnlineStorageService::GetCustomStoragePath(const TArray<FString>& CustomPathArray, bool bCreateIfMissing)
{
    FString BasePath = FPaths::ProjectSavedDir();
    FString FullPath = BasePath;
    
    for (const FString& Dir : CustomPathArray)
    {
        FullPath = FPaths::Combine(FullPath, Dir);
    }
    
    if (bCreateIfMissing && !FPaths::DirectoryExists(FullPath))
    {
        IFileManager::Get().MakeDirectory(*FullPath, true);
    }
    
    return FullPath;
}

void UOnlineStorageService::InitializeStorageDirectories()
{
    for (uint32 i = 0; i < static_cast<uint32>(EStorageCategory::VehicleProfiles) + 1; i++)
    {
        FString CategoryKey = StaticEnum<EStorageCategory>()->GetNameStringByValue(i);
        TArray<FString> PathArray = LocalArchiveBaseDirs;
        PathArray.Add(CategoryKey);
        StorageDirectories.Add(CategoryKey, PathArray);
    }
}

EOS_ProductUserId UOnlineStorageService::GetAuthenticatedUserId() const
{
    if (!SessionAuthenticator)
    {
        return nullptr;
    }
    return SessionAuthenticator->GetAuthenticatedProductUserId();
}

bool UOnlineStorageService::ValidateServiceReady() const
{
    if (!IsStorageInitialized())
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Storage service not initialized"));
        return false;
    }
    
    if (!SessionAuthenticator || SessionAuthenticator->GetEOSLoginStatus() != EOS_ELoginStatus::EOS_LS_LoggedIn)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("User not authenticated"));
        return false;
    }
    
    return true;
}

bool UOnlineStorageService::UploadFileFromPath(const FString& FileName, const FString& FilePath,
    const FOnFileTransferProgressDelegate& OnProgress, const FOnFileTransferCompleteDelegate& OnComplete)
{
    if (!ValidateServiceReady()) return false;
    
    EOS_ProductUserId UserId = GetAuthenticatedUserId();
    if (!UserId) return false;
    
    return UploadFile(UserId, FileName, FilePath, 4096, OnProgress, OnComplete);
}

bool UOnlineStorageService::UploadFileFromData(const FString& FileName, const TArray<uint8>& FileData,
    const FOnFileTransferProgressDelegate& OnProgress, const FOnFileTransferCompleteDelegate& OnComplete)
{
    if (!ValidateServiceReady()) return false;
    
    EOS_ProductUserId UserId = GetAuthenticatedUserId();
    if (!UserId) return false;
    
    return UploadFileInternal(UserId, FileName, FileData, 4096, OnProgress, OnComplete);
}

bool UOnlineStorageService::UploadFile(EOS_ProductUserId LocalUserId, const FString& FileName, 
    const FString& FilePath, uint32 ChunkSizeBytes, FOnFileTransferProgressDelegate OnProgress, 
    FOnFileTransferCompleteDelegate OnComplete)
{
    if (EOS_ProductUserId_IsValid(LocalUserId) == EOS_FALSE)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Invalid local user ID"));
        return false;
    }
    
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Failed to read file: %s"), *FilePath);
        return false;
    }
    
    return UploadFileInternal(LocalUserId, FileName, FileData, ChunkSizeBytes, OnProgress, OnComplete);
}

bool UOnlineStorageService::UploadFileInternal(EOS_ProductUserId LocalUserId, const FString& FileName,
    const TArray<uint8>& FileData, uint32 ChunkSizeBytes, const FOnFileTransferProgressDelegate& OnProgress,
    const FOnFileTransferCompleteDelegate& OnComplete)
{
    if (ActiveTransfers.Contains(FileName))
    {
        UE_LOG(LogOnlineStorage, Warning, TEXT("File already being transferred: %s"), *FileName);
        return false;
    }
    
    FEOSFileTransferContext* Context = new FEOSFileTransferContext();
    Context->FileData = FileData;
    Context->FileName = FileName;
    Context->OnProgress = OnProgress;
    Context->OnComplete = OnComplete;
    Context->UserPointer = this;
    
    EOS_PlayerDataStorage_WriteFileOptions Options = {};
    Options.ApiVersion = EOS_PLAYERDATASTORAGE_WRITEFILE_API_LATEST;
    Options.LocalUserId = LocalUserId;
    
    FTCHARToUTF8 FileNameUtf8(*FileName);
    Options.Filename = FileNameUtf8.Get();
    Options.ChunkLengthBytes = (ChunkSizeBytes > 0) ? ChunkSizeBytes : 4096;
    Options.WriteFileDataCallback = OnWriteFileDataCallback;
    Options.FileTransferProgressCallback = OnFileTransferProgressCallback;
    
    EOS_HPlayerDataStorageFileTransferRequest TransferHandle = EOS_PlayerDataStorage_WriteFile(
        PlayerDataStorageHandle, &Options, Context, OnWriteFileCompleteCallback);
    
    if (!TransferHandle)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Failed to start upload: %s"), *FileName);
        delete Context;
        return false;
    }
    
    ActiveTransfers.Add(FileName, TransferHandle);
    UE_LOG(LogOnlineStorage, Log, TEXT("Started upload: %s (%d bytes)"), *FileName, FileData.Num());
    return true;
}

bool UOnlineStorageService::DownloadFile(const FString& FileName, 
    const FOnFileTransferProgressDelegate& OnProgress, const FOnFileTransferCompleteDelegate& OnComplete)
{
    if (!ValidateServiceReady()) return false;
    
    EOS_ProductUserId UserId = GetAuthenticatedUserId();
    if (!UserId)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Invalid user ID for download"));
        return false;
    }
    
    if (ActiveTransfers.Contains(FileName))
    {
        UE_LOG(LogOnlineStorage, Warning, TEXT("File already being transferred: %s"), *FileName);
        return false;
    }
    
    FEOSFileTransferContext* Context = new FEOSFileTransferContext();
    Context->FileName = FileName;
    Context->OnProgress = OnProgress;
    Context->OnComplete = OnComplete;
    Context->UserPointer = this;
    
    EOS_PlayerDataStorage_ReadFileOptions ReadOptions = {};
    ReadOptions.ApiVersion = EOS_PLAYERDATASTORAGE_READFILE_API_LATEST;
    ReadOptions.LocalUserId = UserId;
    
    std::string Utf8FileName = TCHAR_TO_UTF8(*FileName);
    ReadOptions.Filename = Utf8FileName.c_str();
    ReadOptions.ReadChunkLengthBytes = 4096;
    ReadOptions.ReadFileDataCallback = OnReadFileDataCallback;
    ReadOptions.FileTransferProgressCallback = OnFileTransferProgressCallback;
    
    EOS_HPlayerDataStorageFileTransferRequest TransferHandle = 
        EOS_PlayerDataStorage_ReadFile(PlayerDataStorageHandle, &ReadOptions, Context, OnReadFileCompleteCallback);
        
    if (!TransferHandle)
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Failed to start download: %s"), *FileName);
        delete Context;
        return false;
    }
    
    ActiveTransfers.Add(FileName, TransferHandle);
    UE_LOG(LogOnlineStorage, Log, TEXT("Started download: %s"), *FileName);
    return true;
}

bool UOnlineStorageService::DeleteFile(const FString& FileName, const FOnFileDeleteCompleteDelegate& OnDeleteCompleteDelegate)
{
    if (!ValidateServiceReady()) return false;
    
    EOS_ProductUserId UserId = GetAuthenticatedUserId();
    if (!UserId) return false;
    
    FFileDeleteContext* Context = new FFileDeleteContext();
    Context->UserPointer = this;
    Context->FileName = FileName;
    Context->OnComplete = OnDeleteCompleteDelegate;
    
    EOS_PlayerDataStorage_DeleteFileOptions Options = {};
    Options.ApiVersion = EOS_PLAYERDATASTORAGE_DELETEFILE_API_LATEST;
    Options.LocalUserId = UserId;
    Options.Filename = TCHAR_TO_UTF8(*FileName);
    
    EOS_PlayerDataStorage_DeleteFile(PlayerDataStorageHandle, &Options, Context, OnDeleteFileCompleteCallback);
    
    UE_LOG(LogOnlineStorage, Log, TEXT("Started delete: %s"), *FileName);
    return true;
}

bool UOnlineStorageService::QueryFileList(const FOnFileListQueryCompleteDelegate& OnQueryCompleteDelegate)
{
    if (!ValidateServiceReady()) return false;
    
    EOS_ProductUserId UserId = GetAuthenticatedUserId();
    if (!UserId) return false;
    
    FFileListQueryContext* Context = new FFileListQueryContext();
    Context->UserPointer = this;
    Context->OnComplete = OnQueryCompleteDelegate;
    
    EOS_PlayerDataStorage_QueryFileListOptions Options = {};
    Options.ApiVersion = EOS_PLAYERDATASTORAGE_QUERYFILELIST_API_LATEST;
    Options.LocalUserId = UserId;
    
    EOS_PlayerDataStorage_QueryFileList(PlayerDataStorageHandle, &Options, Context, OnQueryFileListCompleteCallback);
    
    UE_LOG(LogOnlineStorage, Verbose, TEXT("Started file list query"));
    return true;
}

bool UOnlineStorageService::CancelFileTransfer(const FString& FileName)
{
    EOS_HPlayerDataStorageFileTransferRequest* TransferHandle = ActiveTransfers.Find(FileName);
    if (!TransferHandle || !*TransferHandle)
    {
        UE_LOG(LogOnlineStorage, Warning, TEXT("No active transfer for: %s"), *FileName);
        return false;
    }
    
    EOS_EResult CancelResult = EOS_PlayerDataStorageFileTransferRequest_CancelRequest(*TransferHandle);
    EOS_PlayerDataStorageFileTransferRequest_Release(*TransferHandle);
    ActiveTransfers.Remove(FileName);
    
    UE_LOG(LogOnlineStorage, Log, TEXT("Canceled transfer: %s"), *FileName);
    return (CancelResult == EOS_EResult::EOS_Success);
}

TArray<FString> UOnlineStorageService::GetActiveTransfers() const
{
    TArray<FString> ActiveFiles;
    ActiveTransfers.GetKeys(ActiveFiles);
    return ActiveFiles;
}

bool UOnlineStorageService::IsFileBeingTransferred(const FString& FileName) const
{
    return ActiveTransfers.Contains(FileName);
}

void UOnlineStorageService::ClearLocalCache()
{
    if (FPaths::DirectoryExists(LocalCacheDirectory))
    {
        IFileManager::Get().DeleteDirectory(*LocalCacheDirectory, false, true);
        IFileManager::Get().MakeDirectory(*LocalCacheDirectory, true);
        UE_LOG(LogOnlineStorage, Log, TEXT("Cleared local cache directory"));
    }
}

void UOnlineStorageService::ProcessCompletedDownload(const FString& FileName, const TArray<uint8>& FileData, bool bSuccess)
{
    if (bSuccess)
    {
        CacheDownloadedFile(FileName, FileData);
        
        if (ActiveTransfers.Contains(FileName))
        {
            EOS_HPlayerDataStorageFileTransferRequest TransferHandle = ActiveTransfers[FileName];
            if (TransferHandle)
            {
                EOS_PlayerDataStorageFileTransferRequest_Release(TransferHandle);
            }
            ActiveTransfers.Remove(FileName);
        }
        
        UE_LOG(LogOnlineStorage, Log, TEXT("Download completed successfully: %s"), *FileName);
    }
    else
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Download failed: %s"), *FileName);
    }
}

void UOnlineStorageService::CacheDownloadedFile(const FString& FileName, const TArray<uint8>& FileData)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    if (!PlatformFile.DirectoryExists(*LocalCacheDirectory))
    {
        PlatformFile.CreateDirectory(*LocalCacheDirectory);
    }
    
    FString FilePath = FPaths::Combine(LocalCacheDirectory, FileName);
    
    if (FFileHelper::SaveArrayToFile(FileData, *FilePath))
    {
        UE_LOG(LogOnlineStorage, Verbose, TEXT("Cached file: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogOnlineStorage, Error, TEXT("Failed to cache file: %s"), *FilePath);
    }
}

void UOnlineStorageService::RemoveFromLocalCache(const FString& FileName)
{
    if (ActiveTransfers.Contains(FileName))
    {
        EOS_HPlayerDataStorageFileTransferRequest TransferHandle = ActiveTransfers[FileName];
        if (TransferHandle)
        {
            EOS_PlayerDataStorageFileTransferRequest_Release(TransferHandle);
        }
        ActiveTransfers.Remove(FileName);
    }
    
    FString CachePath = FPaths::Combine(LocalCacheDirectory, FileName);
    if (FPaths::FileExists(CachePath))
    {
        IFileManager::Get().Delete(*CachePath);
        UE_LOG(LogOnlineStorage, Verbose, TEXT("Removed cached file: %s"), *CachePath);
    }
}