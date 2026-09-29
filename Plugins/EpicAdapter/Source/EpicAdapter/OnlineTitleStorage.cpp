// OnlineTitleStorage.cpp
#include "OnlineTitleStorage.h"
#include "eos_common.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogOnlineTitleStorage, Log, All);

UOnlineTitleStorage::UOnlineTitleStorage()
    : TitleStorageHandle(nullptr)
    , CurrentTransferHandle(nullptr)
    , CurrentDownloadProgress(0.0f)
    , CurrentDownloadTotalSize(0)
    , CurrentDownloadBytesReceived(0)
{
}

void UOnlineTitleStorage::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ServiceLauncher = GEngine->GetEngineSubsystem<UOnlineServiceLauncher>();
    SessionAuthenticator = GetGameInstance()->GetSubsystem<UOnlineSessionAuthenticator>();

    if (!InitializeEOSHandles())
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("Failed to initialize EOS Title Storage"));
    }
}

void UOnlineTitleStorage::Deinitialize()
{
    CleanupCurrentTransfer();
    CachedFiles.Empty();
    FileMetadataCache.Empty();
    Super::Deinitialize();
}

bool UOnlineTitleStorage::InitializeEOSHandles()
{
    if (!ServiceLauncher || !SessionAuthenticator)
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("Required subsystems not available"));
        return false;
    }

    EOS_HPlatform Platform = ServiceLauncher->GetPlatformHandle();
    if (!Platform)
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("Platform handle not available"));
        return false;
    }

    TitleStorageHandle = EOS_Platform_GetTitleStorageInterface(Platform);
    return TitleStorageHandle != nullptr;
}

bool UOnlineTitleStorage::QueryFileList(const TArray<FString>& Tags)
{
    return QueryFileListInternal(Tags);
}

bool UOnlineTitleStorage::QueryAllFiles()
{
    return QueryFileListInternal(TArray<FString>());
}

bool UOnlineTitleStorage::QueryFileListInternal(const TArray<FString>& Tags)
{
    if (!TitleStorageHandle)
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("Title Storage handle not initialized"));
        return false;
    }

    EOS_ProductUserId LocalUserId = SessionAuthenticator ? SessionAuthenticator->GetAuthenticatedProductUserId() : nullptr;
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("No authenticated user"));
        return false;
    }

    // Convert tags to C-style array
    TArray<const char*> TagsArray;
    TArray<FString> TagsUTF8; // Keep strings alive
    for (const FString& Tag : Tags)
    {
        TagsUTF8.Add(Tag);
        TagsArray.Add(TCHAR_TO_UTF8(*TagsUTF8.Last()));
    }

    EOS_TitleStorage_QueryFileListOptions Options = {};
    Options.ApiVersion = EOS_TITLESTORAGE_QUERYFILELIST_API_LATEST;
    Options.LocalUserId = LocalUserId;
    Options.ListOfTags = TagsArray.Num() > 0 ? TagsArray.GetData() : nullptr;
    Options.ListOfTagsCount = TagsArray.Num();

    EOS_TitleStorage_QueryFileList(TitleStorageHandle, &Options, this, &UOnlineTitleStorage::OnFileListRetrievedCallback);

    UE_LOG(LogOnlineTitleStorage, Log, TEXT("Querying file list with %d tags"), Tags.Num());
    return true;
}

bool UOnlineTitleStorage::DownloadFile(const FString& FileName)
{
    if (!TitleStorageHandle || FileName.IsEmpty())
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("Invalid parameters for file download"));
        return false;
    }

    EOS_ProductUserId LocalUserId = SessionAuthenticator ? SessionAuthenticator->GetAuthenticatedProductUserId() : nullptr;
    if (!LocalUserId)
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("No authenticated user"));
        return false;
    }

    // Cancel any existing transfer
    CleanupCurrentTransfer();

    // Setup new transfer
    CurrentDownloadFileName = FileName;
    CurrentDownloadProgress = 0.0f;
    CurrentDownloadData.Empty();
    CurrentDownloadTotalSize = 0;
    CurrentDownloadBytesReceived = 0;

    EOS_TitleStorage_ReadFileOptions Options = {};
    Options.ApiVersion = EOS_TITLESTORAGE_READFILE_API_LATEST;
    Options.LocalUserId = LocalUserId;
    Options.Filename = TCHAR_TO_UTF8(*FileName);
    Options.ReadChunkLengthBytes = 4 * 4096; // 16KB chunks
    Options.ReadFileDataCallback = &UOnlineTitleStorage::OnFileDataReceivedCallback;
    Options.FileTransferProgressCallback = &UOnlineTitleStorage::OnFileTransferProgressCallback;

    CurrentTransferHandle = EOS_TitleStorage_ReadFile(TitleStorageHandle, &Options, this, &UOnlineTitleStorage::OnFileDownloadCompleteCallback);

    if (!CurrentTransferHandle)
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("Failed to start download for file: %s"), *FileName);
        return false;
    }

    UE_LOG(LogOnlineTitleStorage, Log, TEXT("Started download for file: %s"), *FileName);
    return true;
}

FString UOnlineTitleStorage::GetCachedFileContent(const FString& FileName, bool& bIsAvailable)
{
    if (CachedFiles.Contains(FileName))
    {
        bIsAvailable = true;
        return CachedFiles[FileName];
    }

    bIsAvailable = false;
    return FString();
}

FTitleStorageFileInfo UOnlineTitleStorage::GetFileInfo(const FString& FileName)
{
    if (FileMetadataCache.Contains(FileName))
    {
        return FileMetadataCache[FileName];
    }

    return FTitleStorageFileInfo();
}

TArray<FString> UOnlineTitleStorage::GetCachedFileNames() const
{
    TArray<FString> FileNames;
    CachedFiles.GetKeys(FileNames);
    return FileNames;
}

void UOnlineTitleStorage::CancelCurrentTransfer()
{
    if (CurrentTransferHandle)
    {
        EOS_TitleStorageFileTransferRequest_CancelRequest(CurrentTransferHandle);
        UE_LOG(LogOnlineTitleStorage, Log, TEXT("Cancelled current transfer for: %s"), *CurrentDownloadFileName);
    }
    CleanupCurrentTransfer();
}

void UOnlineTitleStorage::ClearCache()
{
    CachedFiles.Empty();
    FileMetadataCache.Empty();
    UE_LOG(LogOnlineTitleStorage, Log, TEXT("Cleared title storage cache"));
}

void UOnlineTitleStorage::CleanupCurrentTransfer()
{
    if (CurrentTransferHandle)
    {
        EOS_TitleStorageFileTransferRequest_Release(CurrentTransferHandle);
        CurrentTransferHandle = nullptr;
    }

    CurrentDownloadFileName.Empty();
    CurrentDownloadProgress = 0.0f;
    CurrentDownloadData.Empty();
    CurrentDownloadTotalSize = 0;
    CurrentDownloadBytesReceived = 0;
}

//
// Static EOS Callbacks
//

void EOS_CALL UOnlineTitleStorage::OnFileListRetrievedCallback(const EOS_TitleStorage_QueryFileListCallbackInfo* Data)
{
    UOnlineTitleStorage* Self = static_cast<UOnlineTitleStorage*>(Data->ClientData);
    if (!Self || !IsValid(Self))
    {
        return;
    }

    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    TArray<FString> FileNames;

    if (bSuccess)
    {
        UE_LOG(LogOnlineTitleStorage, Log, TEXT("File list query successful, found %d files"), Data->FileCount);

        // Get file metadata for each file
        EOS_ProductUserId LocalUserId = Self->SessionAuthenticator ? Self->SessionAuthenticator->GetAuthenticatedProductUserId() : nullptr;
        if (LocalUserId)
        {
            for (uint32 i = 0; i < Data->FileCount; ++i)
            {
                EOS_TitleStorage_CopyFileMetadataAtIndexOptions Options = {};
                Options.ApiVersion = EOS_TITLESTORAGE_COPYFILEMETADATAATINDEX_API_LATEST;
                Options.LocalUserId = LocalUserId;
                Options.Index = i;

                EOS_TitleStorage_FileMetadata* FileMetadata = nullptr;
                EOS_EResult Result = EOS_TitleStorage_CopyFileMetadataAtIndex(Self->TitleStorageHandle, &Options, &FileMetadata);

                if (Result == EOS_EResult::EOS_Success && FileMetadata)
                {
                    FString FileName = UTF8_TO_TCHAR(FileMetadata->Filename);
                    FileNames.Add(FileName);

                    // Cache metadata
                    FTitleStorageFileInfo& FileInfo = Self->FileMetadataCache.FindOrAdd(FileName);
                    FileInfo.FileName = FileName;
                    FileInfo.FileSizeBytes = FileMetadata->FileSizeBytes;
                    FileInfo.MD5Hash = UTF8_TO_TCHAR(FileMetadata->MD5Hash);
                    FileInfo.UnencryptedDataSizeBytes = FileMetadata->UnencryptedDataSizeBytes;

                    EOS_TitleStorage_FileMetadata_Release(FileMetadata);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("File list query failed: %s"), UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
    }

    Self->OnFileListQueriedDelegate.Broadcast(bSuccess, FileNames);
}

EOS_TitleStorage_EReadResult EOS_CALL UOnlineTitleStorage::OnFileDataReceivedCallback(const EOS_TitleStorage_ReadFileDataCallbackInfo* Data)
{
    UOnlineTitleStorage* Self = static_cast<UOnlineTitleStorage*>(Data->ClientData);
    if (!Self || !IsValid(Self) || !Data->DataChunk)
    {
        return EOS_TitleStorage_EReadResult::EOS_TS_RR_FailRequest;
    }

    // Initialize download buffer on first chunk
    if (Self->CurrentDownloadTotalSize == 0)
    {
        Self->CurrentDownloadTotalSize = Data->TotalFileSizeBytes;
        Self->CurrentDownloadData.Reserve(Self->CurrentDownloadTotalSize);
    }

    // Append data chunk
    const uint8* ChunkData = static_cast<const uint8*>(Data->DataChunk);
    Self->CurrentDownloadData.Append(ChunkData, Data->DataChunkLengthBytes);
    Self->CurrentDownloadBytesReceived += Data->DataChunkLengthBytes;

    return EOS_TitleStorage_EReadResult::EOS_TS_RR_ContinueReading;
}

void EOS_CALL UOnlineTitleStorage::OnFileDownloadCompleteCallback(const EOS_TitleStorage_ReadFileCallbackInfo* Data)
{
    UOnlineTitleStorage* Self = static_cast<UOnlineTitleStorage*>(Data->ClientData);
    if (!Self || !IsValid(Self))
    {
        return;
    }

    bool bSuccess = (Data->ResultCode == EOS_EResult::EOS_Success);
    FString FileName = UTF8_TO_TCHAR(Data->Filename);
    FString FileContent;

    if (bSuccess && Self->CurrentDownloadData.Num() > 0)
    {
        // Convert binary data to string (assumes text files)
        // For binary files, you might want to base64 encode or handle differently
        FileContent = FString(FUTF8ToTCHAR(reinterpret_cast<const ANSICHAR*>(Self->CurrentDownloadData.GetData()), Self->CurrentDownloadData.Num()));
        
        // Cache the file content
        Self->CachedFiles.Add(FileName, FileContent);
        
        UE_LOG(LogOnlineTitleStorage, Log, TEXT("File download successful: %s (%d bytes)"), *FileName, Self->CurrentDownloadData.Num());
    }
    else
    {
        UE_LOG(LogOnlineTitleStorage, Error, TEXT("File download failed: %s - %s"), *FileName, UTF8_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
    }

    Self->OnFileDownloadedDelegate.Broadcast(bSuccess, FileName, FileContent);
    Self->CleanupCurrentTransfer();
}

void EOS_CALL UOnlineTitleStorage::OnFileTransferProgressCallback(const EOS_TitleStorage_FileTransferProgressCallbackInfo* Data)
{
    UOnlineTitleStorage* Self = static_cast<UOnlineTitleStorage*>(Data->ClientData);
    if (!Self || !IsValid(Self))
    {
        return;
    }

    if (Data->TotalFileSizeBytes > 0)
    {
        Self->CurrentDownloadProgress = static_cast<float>(Data->BytesTransferred) / Data->TotalFileSizeBytes;
    }

    FString FileName = UTF8_TO_TCHAR(Data->Filename);
    Self->OnDownloadProgressDelegate.Broadcast(FileName, Self->CurrentDownloadProgress, Data->BytesTransferred);
}