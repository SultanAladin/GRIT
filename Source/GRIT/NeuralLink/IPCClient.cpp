#include "IPCClient.h"
#include "HAL/PlatformMisc.h"
#include <cstring>
#include <cstdio>
#include <vector>

// Reuse the shared memory structures from the server side
// These must match exactly
namespace Cortex
{

#pragma pack(push, 1)
struct ClientRingHeader
{
    volatile uint32_t WritePos;
    volatile uint32_t ReadPos;
    uint32_t Capacity;
    uint32_t Reserved;
};

struct ClientSharedMemoryHeader
{
    uint32_t Magic;
    uint32_t Version;
    uint32_t TotalSize;
    uint32_t TxRingOffset;
    uint32_t TxRingSize;
    uint32_t RxRingOffset;
    uint32_t RxRingSize;
    volatile uint32_t ServerPID;
    volatile uint32_t ClientPID;
    volatile int64_t LastServerHeartbeat;
    volatile int64_t LastClientHeartbeat;
    uint32_t Padding[4];
};

struct ClientMessageHeader
{
    uint8_t Type;
    uint8_t Flags;
    uint16_t EntityCount;
    uint32_t PayloadSize;
    uint32_t SequenceNum;
    uint32_t Timestamp;
};
#pragma pack(pop)

static constexpr uint32_t CLIENT_IPC_MAGIC = 0x50495843;

IPCClient::~IPCClient()
{
    Disconnect();
}

bool IPCClient::Connect(const char* ChannelName)
{
    FileMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, ChannelName);
    if (!FileMapping) return false;

    MappedPtr = MapViewOfFile(FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!MappedPtr)
    {
        CloseHandle(FileMapping);
        FileMapping = nullptr;
        return false;
    }

    auto* Header = static_cast<ClientSharedMemoryHeader*>(MappedPtr);
    if (Header->Magic != CLIENT_IPC_MAGIC) { Disconnect(); return false; }

    Header->ClientPID = GetCurrentProcessId();

    char TxName[128], RxName[128];
    snprintf(TxName, sizeof(TxName), "%s_TxReady", ChannelName);
    snprintf(RxName, sizeof(RxName), "%s_RxReady", ChannelName);

    TxSemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, 0, TxName);
    RxSemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, 0, RxName);

    SequenceNum = 0;
    return true;
}

void IPCClient::Disconnect()
{
    if (MappedPtr) { UnmapViewOfFile(MappedPtr); MappedPtr = nullptr; }
    if (FileMapping) { CloseHandle(FileMapping); FileMapping = nullptr; }
    if (TxSemaphore) { CloseHandle(TxSemaphore); TxSemaphore = nullptr; }
    if (RxSemaphore) { CloseHandle(RxSemaphore); RxSemaphore = nullptr; }
}

static bool WriteToTxRing(void* MappedPtr, const void* Data, uint32_t Size)
{
    auto* ShmHeader = static_cast<ClientSharedMemoryHeader*>(MappedPtr);
    auto* Ring = reinterpret_cast<ClientRingHeader*>(static_cast<uint8_t*>(MappedPtr) + ShmHeader->TxRingOffset);
    uint8_t* RingData = reinterpret_cast<uint8_t*>(Ring) + sizeof(ClientRingHeader);

    uint32_t TotalWrite = sizeof(uint32_t) + Size;
    uint32_t WritePos = Ring->WritePos;
    uint32_t ReadPos = Ring->ReadPos;
    uint32_t Used = (WritePos >= ReadPos) ? (WritePos - ReadPos) : (Ring->Capacity - ReadPos + WritePos);
    uint32_t Free = Ring->Capacity - Used - 1;

    if (TotalWrite > Free) return false;

    uint32_t Pos = WritePos;
    // Write size
    for (uint32_t i = 0; i < sizeof(uint32_t); ++i)
    {
        RingData[Pos % Ring->Capacity] = ((const uint8_t*)&Size)[i];
        ++Pos;
    }
    // Write data
    const uint8_t* Src = static_cast<const uint8_t*>(Data);
    for (uint32_t i = 0; i < Size; ++i)
    {
        RingData[Pos % Ring->Capacity] = Src[i];
        ++Pos;
    }

    FPlatformMisc::MemoryBarrier();
    Ring->WritePos = Pos % Ring->Capacity;
    return true;
}

static bool ReadFromRxRing(void* MappedPtr, void* OutData, uint32_t MaxSize, uint32_t& OutSize)
{
    auto* ShmHeader = static_cast<ClientSharedMemoryHeader*>(MappedPtr);
    auto* Ring = reinterpret_cast<ClientRingHeader*>(static_cast<uint8_t*>(MappedPtr) + ShmHeader->RxRingOffset);
    uint8_t* RingData = reinterpret_cast<uint8_t*>(Ring) + sizeof(ClientRingHeader);

    uint32_t WritePos = Ring->WritePos;
    uint32_t ReadPos = Ring->ReadPos;
    FPlatformMisc::MemoryBarrier();

    if (WritePos == ReadPos) { OutSize = 0; return false; }

    uint32_t MsgSize = 0;
    uint32_t Pos = ReadPos;
    for (uint32_t i = 0; i < sizeof(uint32_t); ++i)
    {
        ((uint8_t*)&MsgSize)[i] = RingData[Pos % Ring->Capacity];
        ++Pos;
    }

    if (MsgSize > MaxSize) { OutSize = 0; return false; }

    uint8_t* Dst = static_cast<uint8_t*>(OutData);
    for (uint32_t i = 0; i < MsgSize; ++i)
    {
        Dst[i] = RingData[Pos % Ring->Capacity];
        ++Pos;
    }

    FPlatformMisc::MemoryBarrier();
    Ring->ReadPos = Pos % Ring->Capacity;
    OutSize = MsgSize;
    return true;
}

bool IPCClient::SendObservations(const FObservationVector* Observations, uint32_t Count)
{
    if (!MappedPtr) return false;

    // Pack: header + observations
    uint32_t PayloadSize = Count * sizeof(FObservationVector);
    uint32_t TotalSize = sizeof(ClientMessageHeader) + PayloadSize;
    std::vector<uint8_t> Buf(TotalSize);

    ClientMessageHeader Hdr;
    Hdr.Type = 0x01; // Observe
    Hdr.Flags = 0;
    Hdr.EntityCount = (uint16_t)Count;
    Hdr.PayloadSize = PayloadSize;
    Hdr.SequenceNum = ++SequenceNum;
    Hdr.Timestamp = 0;

    memcpy(Buf.data(), &Hdr, sizeof(Hdr));
    memcpy(Buf.data() + sizeof(Hdr), Observations, PayloadSize);

    bool Ok = WriteToTxRing(MappedPtr, Buf.data(), TotalSize);
    if (Ok && TxSemaphore)
        ReleaseSemaphore(TxSemaphore, 1, nullptr);
    return Ok;
}

bool IPCClient::ReceiveActions(FActionVector* OutActions, uint32_t MaxCount, uint32_t& OutCount)
{
    if (!MappedPtr) return false;

    uint8_t ReadBuf[65536];
    uint32_t BytesRead;
    if (!ReadFromRxRing(MappedPtr, ReadBuf, sizeof(ReadBuf), BytesRead))
        return false;

    if (BytesRead < sizeof(ClientMessageHeader)) return false;

    ClientMessageHeader Hdr;
    memcpy(&Hdr, ReadBuf, sizeof(Hdr));
    if (Hdr.Type != 0x02) return false; // Not an Act message

    OutCount = Hdr.EntityCount;
    if (OutCount > MaxCount) OutCount = MaxCount;

    memcpy(OutActions, ReadBuf + sizeof(Hdr), OutCount * sizeof(FActionVector));
    return true;
}

bool IPCClient::SendRewards(const FRewardSignal* Rewards, uint32_t Count)
{
    if (!MappedPtr) return false;

    uint32_t PayloadSize = Count * sizeof(FRewardSignal);
    uint32_t TotalSize = sizeof(ClientMessageHeader) + PayloadSize;
    std::vector<uint8_t> Buf(TotalSize);

    ClientMessageHeader Hdr;
    Hdr.Type = 0x03;
    Hdr.Flags = 0;
    Hdr.EntityCount = (uint16_t)Count;
    Hdr.PayloadSize = PayloadSize;
    Hdr.SequenceNum = ++SequenceNum;
    Hdr.Timestamp = 0;

    memcpy(Buf.data(), &Hdr, sizeof(Hdr));
    memcpy(Buf.data() + sizeof(Hdr), Rewards, PayloadSize);

    bool Ok = WriteToTxRing(MappedPtr, Buf.data(), TotalSize);
    if (Ok && TxSemaphore)
        ReleaseSemaphore(TxSemaphore, 1, nullptr);
    return Ok;
}

bool IPCClient::SendConfig(const FEntityRegistration& Config)
{
    if (!MappedPtr) return false;

    uint32_t TotalSize = sizeof(ClientMessageHeader) + sizeof(FEntityRegistration);
    std::vector<uint8_t> Buf(TotalSize);

    ClientMessageHeader Hdr;
    Hdr.Type = 0x05;
    Hdr.Flags = 0;
    Hdr.EntityCount = 1;
    Hdr.PayloadSize = sizeof(FEntityRegistration);
    Hdr.SequenceNum = ++SequenceNum;
    Hdr.Timestamp = 0;

    memcpy(Buf.data(), &Hdr, sizeof(Hdr));
    memcpy(Buf.data() + sizeof(Hdr), &Config, sizeof(Config));

    return WriteToTxRing(MappedPtr, Buf.data(), TotalSize);
}

void IPCClient::SendHeartbeat()
{
    if (!MappedPtr) return;

    auto* Header = static_cast<ClientSharedMemoryHeader*>(MappedPtr);
    LARGE_INTEGER Now;
    QueryPerformanceCounter(&Now);
    Header->LastClientHeartbeat = Now.QuadPart;
}

bool IPCClient::CheckServerAlive(double TimeoutSec)
{
    if (!MappedPtr) return false;

    auto* Header = static_cast<ClientSharedMemoryHeader*>(MappedPtr);
    LARGE_INTEGER Now, Freq;
    QueryPerformanceCounter(&Now);
    QueryPerformanceFrequency(&Freq);

    if (Header->LastServerHeartbeat == 0) return false;
    double Elapsed = (double)(Now.QuadPart - Header->LastServerHeartbeat) / (double)Freq.QuadPart;
    return Elapsed < TimeoutSec;
}

bool IPCClient::ReceiveStatus(FTrainingStatus& OutStatus)
{
    if (!MappedPtr) return false;

    uint8_t ReadBuf[4096];
    uint32_t BytesRead;
    if (!ReadFromRxRing(MappedPtr, ReadBuf, sizeof(ReadBuf), BytesRead))
        return false;

    if (BytesRead < sizeof(ClientMessageHeader)) return false;

    ClientMessageHeader Hdr;
    memcpy(&Hdr, ReadBuf, sizeof(Hdr));
    if (Hdr.Type != 0x08) return false;

    if (Hdr.PayloadSize >= sizeof(FTrainingStatus))
        memcpy(&OutStatus, ReadBuf + sizeof(Hdr), sizeof(FTrainingStatus));

    return true;
}

void IPCClient::SendShutdown()
{
    if (!MappedPtr) return;

    ClientMessageHeader Hdr;
    Hdr.Type = 0x07;
    Hdr.Flags = 0;
    Hdr.EntityCount = 0;
    Hdr.PayloadSize = 0;
    Hdr.SequenceNum = ++SequenceNum;
    Hdr.Timestamp = 0;

    WriteToTxRing(MappedPtr, &Hdr, sizeof(Hdr));
    if (TxSemaphore)
        ReleaseSemaphore(TxSemaphore, 1, nullptr);
}

} // namespace Cortex
