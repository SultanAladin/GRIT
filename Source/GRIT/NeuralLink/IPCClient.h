#pragma once
// IPCClient.h — Unreal-side shared memory client for CortexServer communication.

#include "CortexTypes.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

namespace Cortex
{

class IPCClient
{
public:
    IPCClient() = default;
    ~IPCClient();

    bool Connect(const char* ChannelName = IPC_CHANNEL_NAME);
    void Disconnect();
    bool IsConnected() const { return MappedPtr != nullptr; }

    bool SendObservations(const FObservationVector* Observations, uint32_t Count);
    bool ReceiveActions(FActionVector* OutActions, uint32_t MaxCount, uint32_t& OutCount);
    bool SendRewards(const FRewardSignal* Rewards, uint32_t Count);
    bool SendConfig(const FEntityRegistration& Config);
    void SendHeartbeat();
    bool CheckServerAlive(double TimeoutSec = 5.0);
    bool ReceiveStatus(FTrainingStatus& OutStatus);
    void SendShutdown();

private:
    HANDLE FileMapping = nullptr;
    HANDLE TxSemaphore = nullptr;
    HANDLE RxSemaphore = nullptr;
    void* MappedPtr = nullptr;
    uint32_t SequenceNum = 0;
};

} // namespace Cortex
