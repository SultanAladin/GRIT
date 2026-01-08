#include "VehicleController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "VehicleSolver.h"
#include "GameContext/PlayerTracker.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GameFramework/PlayerState.h"

//------------------------------------------------------------------------------
//                                      logging
//------------------------------------------------------------------------------
DEFINE_LOG_CATEGORY(LogVehicleController);

//------------------------------------------------------------------------------
//                              telemetry capture struct
//------------------------------------------------------------------------------
struct FMultiplayerTelemetry
{
    float Timestamp;                 // [s] - Game time
    FString PlayerName;              // [-] - Player controller name
    bool bIsServer;                  // [-] - Server authority flag
    bool bIsLocalController;         // [-] - Local controller flag
    FString VehicleName;             // [-] - Possessed vehicle name
    bool bVehicleHasOwner;           // [-] - Owner assignment status
    bool bVehicleHasNetConnection;   // [-] - Network connection status
    ENetRole LocalRole;              // [-] - Actor's local role
    ENetRole RemoteRole;             // [-] - Actor's remote role
    float Ping;                      // [ms] - Network latency
    float Throttle;                  // [0..1] - Throttle input
    float Brake;                     // [0..1] - Brake input
    float Steering;                  // [-1..1] - Steering input
    FVector Location;                // [cm] - Vehicle position
    FVector Velocity;                // [cm·s⁻¹] - Vehicle velocity
    float Speed;                     // [km·h⁻¹] - Vehicle speed
    int32 PacketsSent;               // [-] - Network packets transmitted
    int32 PacketsReceived;           // [-] - Network packets received
    float BytesSentPerSec;           // [B·s⁻¹] - Upload bandwidth
    float BytesReceivedPerSec;       // [B·s⁻¹] - Download bandwidth
};

//------------------------------------------------------------------------------
//                              static telemetry buffer
//------------------------------------------------------------------------------
static TArray<FMultiplayerTelemetry> GTelemetryBuffer;
static FCriticalSection GTelemetryMutex;
static bool GbTelemetryEnabled = true;

//------------------------------------------------------------------------------
//                              console commands for telemetry
//------------------------------------------------------------------------------
static FAutoConsoleCommand TelemetryStatusCmd(
    TEXT("GRIT.Telemetry.Status"),
    TEXT("Show telemetry system status"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        UE_LOG(LogVehicleController, Warning, TEXT("📊 TELEMETRY STATUS:"));
        UE_LOG(LogVehicleController, Warning, TEXT("   Enabled: %s"), GbTelemetryEnabled ? TEXT("YES") : TEXT("NO"));
        UE_LOG(LogVehicleController, Warning, TEXT("   Buffer Size: %d samples"), GTelemetryBuffer.Num());
        UE_LOG(LogVehicleController, Warning, TEXT("   Save Location: %s"), *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GRIT/VehicleFramework/Telemetry")));
        
        // Reason: provide visual feedback in-game
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("📊 Telemetry: %s | Buffer: %d samples"), GbTelemetryEnabled ? TEXT("ENABLED") : TEXT("DISABLED"), GTelemetryBuffer.Num()));
        } // End if (engine available)
    })
);

static FAutoConsoleCommand TelemetryToggleCmd(
    TEXT("GRIT.Telemetry.Toggle"),
    TEXT("Toggle telemetry on/off"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        GbTelemetryEnabled = !GbTelemetryEnabled;
        UE_LOG(LogVehicleController, Warning, TEXT("📊 Telemetry %s"), GbTelemetryEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
        
        // Reason: immediate visual confirmation
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, GbTelemetryEnabled ? FColor::Green : FColor::Red, FString::Printf(TEXT("📊 Telemetry %s"), GbTelemetryEnabled ? TEXT("ENABLED") : TEXT("DISABLED")));
        } // End if (engine available)
    })
);

/*====================================================================================================================================
                                                         CONTROLLER LIFECYCLE
======================================================================================================================================*/

AVehicleController::AVehicleController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    UE_LOG(LogVehicleController, Log, TEXT("🎮 Controller bootstrapped | %s"), *GetName());
}

void AVehicleController::BeginPlay()
{
    Super::BeginPlay();

    EstablishVehicleConnection();

    // Reason: inject enhanced input mapping context for vehicle controls
    if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            // Reason: guard against null mapping context
            if (VehicleInputMappingContext)
            {
                Subsystem->AddMappingContext(VehicleInputMappingContext, 0);
                UE_LOG(LogVehicleController, Log, TEXT("🕹️ Mapping context injected | %s"), *GetName());
            }
            else
            {
                UE_LOG(LogVehicleController, Warning, TEXT("⚠️ Mapping context null | %s"), *GetName());
            } // End if (mapping context check)
        } // End if (subsystem available)
    } // End if (local player check)
}

void AVehicleController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAnalogInputs(DeltaTime);

    //------------------------------------------------------------------------------
    // route inputs based on network role
    //------------------------------------------------------------------------------
    if (ControlledVehicle)
    {
        const bool bIsServer = HasAuthority();                     // [-] - Authority status
        const bool bIsLocal = IsLocalController();                 // [-] - Local controller status

        // Reason: clients send input via RPC, server applies locally
        if (bIsLocal && !bIsServer)
        {
            // Reason: only send when input changes (delta compression)
            if (!PendingInput.NearlyEquals(LastSentInput, 1.e-3f))
            {
                ServerSendInput(PendingInput);
                LastSentInput = PendingInput;
            } // End if (input changed)
        }
        else if (bIsServer)
        {
            // Reason: server with local player applies input directly
            if (bIsLocal)
            {
                ControlledVehicle->InputTensor_GameThread = PendingInput;
            }
        } // End if (role-based routing)
    } // End if (vehicle valid)

    //------------------------------------------------------------------------------
    // capture multiplayer telemetry every frame
    //------------------------------------------------------------------------------
    if (GbTelemetryEnabled && ControlledVehicle)
    {
        FMultiplayerTelemetry Sample;
        Sample.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;          // [s]
        Sample.PlayerName = GetName();
        Sample.bIsServer = GetLocalRole() == ROLE_Authority;
        Sample.bIsLocalController = IsLocalController();
        Sample.VehicleName = ControlledVehicle->GetName();
        Sample.bVehicleHasOwner = (ControlledVehicle->GetOwner() != nullptr);
        Sample.bVehicleHasNetConnection = (ControlledVehicle->GetNetConnection() != nullptr);
        Sample.LocalRole = ControlledVehicle->GetLocalRole();
        Sample.RemoteRole = ControlledVehicle->GetRemoteRole();
        Sample.Ping = PlayerState ? PlayerState->GetPingInMilliseconds() : 0.0f;     // [ms]

        // Reason: capture raw input for local controllers, applied input for remote views
        if (IsLocalController())
        {
            Sample.Throttle = PendingInput.Throttle;
            Sample.Brake = PendingInput.Brake;
            Sample.Steering = PendingInput.Steering;
        }
        else
        {
            Sample.Throttle = ControlledVehicle->InputTensor_GameThread.Throttle;
            Sample.Brake = ControlledVehicle->InputTensor_GameThread.Brake;
            Sample.Steering = ControlledVehicle->InputTensor_GameThread.Steering;
        } // End if (input source selection)
        
        // Reason: capture vehicle physics state
        if (UStaticMeshComponent* Hull = ControlledVehicle->VehicleHull)
        {
            Sample.Location = Hull->GetComponentLocation();                          // [cm]
            Sample.Velocity = Hull->GetPhysicsLinearVelocity();                      // [cm·s⁻¹]
            Sample.Speed = Sample.Velocity.Size() * 0.036f;                          // [km·h⁻¹] (cm·s⁻¹ × 0.036)
        }
        else
        {
            Sample.Location = FVector::ZeroVector;
            Sample.Velocity = FVector::ZeroVector;
            Sample.Speed = 0.0f;
        } // End if (hull exists)

        // Reason: capture network statistics
        if (UNetConnection* NetConn = GetNetConnection())
        {
            Sample.PacketsSent = NetConn->OutPackets;
            Sample.PacketsReceived = NetConn->InPackets;
            Sample.BytesSentPerSec = NetConn->OutBytesPerSecond;                     // [B·s⁻¹]
            Sample.BytesReceivedPerSec = NetConn->InBytesPerSecond;                  // [B·s⁻¹]
        }
        else
        {
            Sample.PacketsSent = 0;
            Sample.PacketsReceived = 0;
            Sample.BytesSentPerSec = 0.0f;
            Sample.BytesReceivedPerSec = 0.0f;
        } // End if (net connection)

        FScopeLock Lock(&GTelemetryMutex);
        GTelemetryBuffer.Add(Sample);
        
        // Reason: periodic logging confirmation every 60 frames
        static int32 TelemetryFrameCount = 0;
        if (++TelemetryFrameCount % 60 == 0)
        {
            UE_LOG(LogVehicleController, Log, TEXT("📊 Telemetry capturing | Player: %s | Samples: %d | Speed: %.1f km/h | Owner: %s | Role: %s"), *Sample.PlayerName, GTelemetryBuffer.Num(), Sample.Speed, Sample.bVehicleHasOwner ? TEXT("YES") : TEXT("NO"), Sample.bIsServer ? TEXT("SERVER") : TEXT("CLIENT"));
        } // End if (logging interval)
    } // End if (telemetry enabled)
}

/*====================================================================================================================================
                                                         INPUT BINDING
======================================================================================================================================*/

void AVehicleController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    
    // Reason: require enhanced input for action binding
    if (!EnhancedInput)
    {
        UE_LOG(LogVehicleController, Error, TEXT("⛔ Enhanced input component missing | %s"), *GetName());
        return;
    } // End if (enhanced input guard)

    if (IA_Throttle)
    {
        EnhancedInput->BindAction(IA_Throttle, ETriggerEvent::Triggered, this, &AVehicleController::ThrottleTriggered);
        EnhancedInput->BindAction(IA_Throttle, ETriggerEvent::Completed, this, &AVehicleController::ThrottleCompleted);
    }

    if (IA_Brake)
    {
        EnhancedInput->BindAction(IA_Brake, ETriggerEvent::Triggered, this, &AVehicleController::BrakeTriggered);
        EnhancedInput->BindAction(IA_Brake, ETriggerEvent::Completed, this, &AVehicleController::BrakeCompleted);
    }

    if (IA_Steer)
    {
        EnhancedInput->BindAction(IA_Steer, ETriggerEvent::Triggered, this, &AVehicleController::SteerTriggered);
        EnhancedInput->BindAction(IA_Steer, ETriggerEvent::Completed, this, &AVehicleController::SteerCompleted);
    }

    if (IA_Handbrake)
    {
        EnhancedInput->BindAction(IA_Handbrake, ETriggerEvent::Triggered, this, &AVehicleController::HandbrakeTriggered);
        EnhancedInput->BindAction(IA_Handbrake, ETriggerEvent::Completed, this, &AVehicleController::HandbrakeCompleted);
    }

    if (IA_GearUp)
    {
        EnhancedInput->BindAction(IA_GearUp, ETriggerEvent::Started, this, &AVehicleController::GearUpStarted);
        EnhancedInput->BindAction(IA_GearUp, ETriggerEvent::Completed, this, &AVehicleController::GearUpCompleted);
    }

    if (IA_GearDown)
    {
        EnhancedInput->BindAction(IA_GearDown, ETriggerEvent::Started, this, &AVehicleController::GearDownStarted);
        EnhancedInput->BindAction(IA_GearDown, ETriggerEvent::Completed, this, &AVehicleController::GearDownCompleted);
    }

    if (IA_Clutch)
    {
        EnhancedInput->BindAction(IA_Clutch, ETriggerEvent::Triggered, this, &AVehicleController::ClutchTriggered);
        EnhancedInput->BindAction(IA_Clutch, ETriggerEvent::Completed, this, &AVehicleController::ClutchCompleted);
    }

    if (IA_OverDrive)
    {
        EnhancedInput->BindAction(IA_OverDrive, ETriggerEvent::Started, this, &AVehicleController::OverDriveStarted);
        EnhancedInput->BindAction(IA_OverDrive, ETriggerEvent::Completed, this, &AVehicleController::OverDriveCompleted);
    }
}

/*====================================================================================================================================
                                                         POSSESSION SYSTEM
======================================================================================================================================*/

void AVehicleController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledVehicle = Cast<AVehicleSolver>(InPawn);
    
    // Reason: validate vehicle possession and establish network ownership
    if (ControlledVehicle)
    {
        //------------------------------------------------------------------------------
        // CRITICAL: set vehicle owner to this controller for RPC routing
        //------------------------------------------------------------------------------
        ControlledVehicle->SetOwner(this);
        
        // Reason: force immediate replication of owner to all clients
        if (GetLocalRole() == ROLE_Authority)
        {
            ControlledVehicle->ForceNetUpdate();
        } // End if (server authority)
        
        bConnectionEstablished = true;
        PendingInput.FlushInputs();
        LastSentInput.FlushInputs();
        bInputDirty = false;
        
        const bool bIsServer = (GetLocalRole() == ROLE_Authority);
        const bool bHasOwner = (ControlledVehicle->GetOwner() != nullptr);
        
        UE_LOG(LogVehicleController, Log, TEXT("✅ Vehicle possession established | Controller: %s | Vehicle: %s | Role: %s | Owner: %s"), *GetName(), *ControlledVehicle->GetName(), bIsServer ? TEXT("SERVER") : TEXT("CLIENT"), bHasOwner ? *ControlledVehicle->GetOwner()->GetName() : TEXT("NONE"));
    }
    else
    {
        bConnectionEstablished = false;
        UE_LOG(LogVehicleController, Warning, TEXT("⚠️ Pawn not a VehicleConstruct | Controller: %s"), *GetName());
    } // End if (possession validation)
}

void AVehicleController::OnUnPossess()
{
    //------------------------------------------------------------------------------
    // write telemetry to CSV before unpossessing
    //------------------------------------------------------------------------------
    if (GbTelemetryEnabled && GTelemetryBuffer.Num() > 0)
    {
        FString ProjectDir = FPaths::ProjectDir();
        FString TelemetryDir = FPaths::Combine(ProjectDir, TEXT("Source/GRIT/VehicleFramework/Telemetry"));
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        
        // Reason: ensure telemetry directory exists
        if (!PlatformFile.DirectoryExists(*TelemetryDir))
        {
            PlatformFile.CreateDirectory(*TelemetryDir);
        } // End if (directory creation)

        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        FString PlayerID = GetName().Replace(TEXT("VehicleController"), TEXT("Player"));
        FString CsvFilename = FString::Printf(TEXT("Multiplayer_%s_%s.csv"), *PlayerID, *Timestamp);
        FString CsvFilePath = FPaths::Combine(TelemetryDir, CsvFilename);

        FString CsvContent = TEXT("Timestamp,PlayerName,IsServer,IsLocalController,VehicleName,HasOwner,HasNetConnection,LocalRole,RemoteRole,Ping_ms,Throttle,Brake,Steering,LocationX,LocationY,LocationZ,VelX,VelY,VelZ,Speed_kmh,PacketsSent,PacketsReceived,BytesSent,BytesReceived\n");

        FString MarkdownContent;

        TMap<FString, int32> SamplesPerPlayer;
        TMap<FString, float> TotalPingPerPlayer;
        TMap<FString, float> MinPingPerPlayer;
        TMap<FString, float> MaxPingPerPlayer;
        TSet<FString> ClientPlayerNames;
        float MinTimestamp = 0.0f;                    // [s] - Earliest sample time
        float MaxTimestamp = 0.0f;                    // [s] - Latest sample time
        bool bHasTimestamp = false;                   // [-] - Timestamp initialization flag

        {
            FScopeLock Lock(&GTelemetryMutex);
            for (const FMultiplayerTelemetry& Sample : GTelemetryBuffer)
            {
                FString RoleStr = (Sample.LocalRole == ROLE_Authority) ? TEXT("Authority") : (Sample.LocalRole == ROLE_AutonomousProxy) ? TEXT("AutonomousProxy") : TEXT("SimulatedProxy");
                FString RemoteRoleStr = (Sample.RemoteRole == ROLE_Authority) ? TEXT("Authority") : (Sample.RemoteRole == ROLE_AutonomousProxy) ? TEXT("AutonomousProxy") : TEXT("SimulatedProxy");
                
                CsvContent += FString::Printf(TEXT("%.3f,%s,%d,%d,%s,%d,%d,%s,%s,%.1f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%d,%d,%.1f,%.1f\n"), Sample.Timestamp, *Sample.PlayerName, Sample.bIsServer ? 1 : 0, Sample.bIsLocalController ? 1 : 0, *Sample.VehicleName, Sample.bVehicleHasOwner ? 1 : 0, Sample.bVehicleHasNetConnection ? 1 : 0, *RoleStr, *RemoteRoleStr, Sample.Ping, Sample.Throttle, Sample.Brake, Sample.Steering, Sample.Location.X, Sample.Location.Y, Sample.Location.Z, Sample.Velocity.X, Sample.Velocity.Y, Sample.Velocity.Z, Sample.Speed, Sample.PacketsSent, Sample.PacketsReceived, Sample.BytesSentPerSec, Sample.BytesReceivedPerSec);

                int32& SampleCount = SamplesPerPlayer.FindOrAdd(Sample.PlayerName);
                SampleCount++;

                float& TotalPing = TotalPingPerPlayer.FindOrAdd(Sample.PlayerName);
                TotalPing += Sample.Ping;                                        // [ms]

                float& MinPing = MinPingPerPlayer.FindOrAdd(Sample.PlayerName);
                float& MaxPing = MaxPingPerPlayer.FindOrAdd(Sample.PlayerName);
                
                // Reason: initialize min/max on first sample
                if (SampleCount == 1)
                {
                    MinPing = Sample.Ping;
                    MaxPing = Sample.Ping;
                }
                else
                {
                    MinPing = FMath::Min(MinPing, Sample.Ping);
                    MaxPing = FMath::Max(MaxPing, Sample.Ping);
                } // End if (ping statistics)

                // Reason: track client players for summary
                if (!Sample.bIsServer && Sample.bIsLocalController)
                {
                    ClientPlayerNames.Add(Sample.PlayerName);
                } // End if (client detection)

                // Reason: establish timestamp range
                if (!bHasTimestamp)
                {
                    MinTimestamp = Sample.Timestamp;
                    MaxTimestamp = Sample.Timestamp;
                    bHasTimestamp = true;
                }
                else
                {
                    MinTimestamp = FMath::Min(MinTimestamp, Sample.Timestamp);
                    MaxTimestamp = FMath::Max(MaxTimestamp, Sample.Timestamp);
                } // End if (timestamp tracking)
            } // End for (telemetry samples)

            const int32 TotalSamples = GTelemetryBuffer.Num();
            const int32 UniquePlayers = SamplesPerPlayer.Num();
            const int32 TotalClients = ClientPlayerNames.Num();

            MarkdownContent += TEXT("# Multiplayer Telemetry Summary\n\n");
            MarkdownContent += FString::Printf(TEXT("- Session Timestamp: `%s`\n"), *Timestamp);
            MarkdownContent += FString::Printf(TEXT("- Controller: `%s`\n"), *GetName());
            MarkdownContent += FString::Printf(TEXT("- Samples: %d\n"), TotalSamples);
            MarkdownContent += FString::Printf(TEXT("- Unique Players: %d\n"), UniquePlayers);
            MarkdownContent += FString::Printf(TEXT("- Total Clients: %d\n"), TotalClients);
            
            // Reason: display time range if valid timestamps exist
            if (bHasTimestamp && MinTimestamp <= MaxTimestamp)
            {
                MarkdownContent += FString::Printf(TEXT("- Time Range (s): %.3f → %.3f\n"), MinTimestamp, MaxTimestamp);
            } // End if (timestamp range)

            MarkdownContent += TEXT("\n## Players\n\n");
            for (const TPair<FString, int32>& Pair : SamplesPerPlayer)
            {
                const FString& PlayerName = Pair.Key;
                const int32 PlayerSamples = Pair.Value;
                const float TotalPing = TotalPingPerPlayer.FindRef(PlayerName);
                const float AvgPing = PlayerSamples > 0 ? TotalPing / PlayerSamples : 0.0f;   // [ms]
                const float MinPing = MinPingPerPlayer.FindRef(PlayerName);                   // [ms]
                const float MaxPing = MaxPingPerPlayer.FindRef(PlayerName);                   // [ms]
                const bool bIsClient = ClientPlayerNames.Contains(PlayerName);

                MarkdownContent += FString::Printf(TEXT("- `%s`  \n  - Role: %s  \n  - Samples: %d  \n  - Ping ms: avg %.1f (min %.1f, max %.1f)\n"), *PlayerName, bIsClient ? TEXT("Client") : TEXT("Server"), PlayerSamples, AvgPing, MinPing, MaxPing);
            } // End for (player statistics)

            GTelemetryBuffer.Empty();
        } // End scope lock

        // Reason: write CSV telemetry data to disk
        if (FFileHelper::SaveStringToFile(CsvContent, *CsvFilePath))
        {
            const int32 TotalSamples = SamplesPerPlayer.Num() > 0 ? 0 : 0;
            UE_LOG(LogVehicleController, Warning, TEXT("📊 MULTIPLAYER TELEMETRY SAVED | File: %s | Samples: %d | Location: %s"), *CsvFilePath, TotalSamples, *TelemetryDir);
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("📊 Telemetry saved: %s"), *CsvFilename));
            } // End if (engine available)
        }
        else
        {
            UE_LOG(LogVehicleController, Error, TEXT("❌ FAILED TO WRITE TELEMETRY | File: %s"), *CsvFilePath);
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("❌ Failed to save telemetry: %s"), *CsvFilename));
            } // End if (engine available)
        } // End if (CSV write)

        // Reason: write markdown summary if content exists
        if (!MarkdownContent.IsEmpty())
        {
            FString MarkdownFilename = FString::Printf(TEXT("Multiplayer_%s_%s.md"), *PlayerID, *Timestamp);
            FString MarkdownFilePath = FPaths::Combine(TelemetryDir, MarkdownFilename);

            if (FFileHelper::SaveStringToFile(MarkdownContent, *MarkdownFilePath))
            {
                UE_LOG(LogVehicleController, Warning, TEXT("📄 MULTIPLAYER SUMMARY SAVED | File: %s"), *MarkdownFilePath);
            }
            else
            {
                UE_LOG(LogVehicleController, Error, TEXT("❌ FAILED TO WRITE TELEMETRY SUMMARY | File: %s"), *MarkdownFilePath);
            } // End if (markdown write)
        } // End if (markdown content exists)
    } // End if (telemetry buffer valid)

    bConnectionEstablished = false;
    ControlledVehicle = nullptr;
    Super::OnUnPossess();
    UE_LOG(LogVehicleController, Log, TEXT("🧩 Vehicle released | Controller: %s"), *GetName());
}

void AVehicleController::OnRep_Pawn()
{
    Super::OnRep_Pawn();

    APawn* NewPawn = GetPawn();
    ControlledVehicle = Cast<AVehicleSolver>(NewPawn);

    // Reason: establish ownership and initialize input state on replication
    if (ControlledVehicle)
    {
        ControlledVehicle->SetOwner(this);
        bConnectionEstablished = true;
        PendingInput.FlushInputs();
        LastSentInput.FlushInputs();
        bInputDirty = false;
    }
    else
    {
        bConnectionEstablished = false;
    } // End if (vehicle cast validation)
}

/*====================================================================================================================================
                                                         NETWORK REPLICATION
======================================================================================================================================*/

void AVehicleController::ServerSendInput_Implementation(FInputTensor NewInput)
{
    // Reason: guard against invalid vehicle reference
    if (!ControlledVehicle)
    {
        UE_LOG(LogVehicleController, Warning, TEXT("⚠️ ServerSendInput called without vehicle | Controller: %s"), *GetName());
        return;
    } // End if (vehicle check)

    ControlledVehicle->ServerUpdateInput(NewInput);
}

bool AVehicleController::ServerSendInput_Validate(FInputTensor NewInput)
{
    return true;
}

/*====================================================================================================================================
                                                         INPUT HANDLERS
======================================================================================================================================*/

void AVehicleController::ThrottleTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    const float InputValue = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);    // [0..1]
    TargetThrottle = InputValue;
}

void AVehicleController::ThrottleCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetThrottle = 0.0f;
}

void AVehicleController::BrakeTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    const float InputValue = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);    // [0..1]
    TargetBrake = InputValue;
}

void AVehicleController::BrakeCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetBrake = 0.0f;
}

void AVehicleController::SteerTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetSteering = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);           // [-1..1]
}

void AVehicleController::SteerCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetSteering = 0.0f;
}

void AVehicleController::HandbrakeTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetHandbrake = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);           // [0..1]
}

void AVehicleController::HandbrakeCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetHandbrake = 0.0f;
}

void AVehicleController::GearUpStarted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.ShiftUp = true;
    bInputDirty = true;
}

void AVehicleController::GearUpCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.ShiftUp = false;
    bInputDirty = true;
}

void AVehicleController::GearDownStarted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.ShiftDown = true;
    bInputDirty = true;
}

void AVehicleController::GearDownCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.ShiftDown = false;
    bInputDirty = true;
}

void AVehicleController::ClutchTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.Clutch = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);       // [0..1]
    bInputDirty = true;
}

void AVehicleController::ClutchCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.Clutch = 0.0f;
    bInputDirty = true;
}

void AVehicleController::OverDriveStarted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.bOverDrive = true;
    bInputDirty = true;
}

void AVehicleController::OverDriveCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    PendingInput.bOverDrive = false;
    bInputDirty = true;
}

/*====================================================================================================================================
                                                         PRIVATE UTILITIES
======================================================================================================================================*/

void AVehicleController::EstablishVehicleConnection()
{
    bConnectionEstablished = false;
}

void AVehicleController::UpdateAnalogInputs(float DeltaTime)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    const float CurrentThrottle = PendingInput.Throttle;                       // [0..1]
    const float CurrentBrake = PendingInput.Brake;                             // [0..1]
    const float CurrentSteering = PendingInput.Steering;                       // [-1..1]
    const float CurrentHandbrake = PendingInput.Handbrake;                     // [0..1]

    //------------------------------------------------------------------------------
    // input smoothing with conflict resolution
    //------------------------------------------------------------------------------
    constexpr float ConflictThreshold = 0.05f;                                 // [-] - Minimum input magnitude for conflict detection
    
    // Reason: throttle dominant mode
    if (TargetThrottle > ConflictThreshold && TargetBrake < ConflictThreshold)
    {
        PendingInput.Brake = 0.0f;
        PendingInput.Throttle = FMath::FInterpTo(CurrentThrottle, TargetThrottle, DeltaTime, ThrottleRate);
        bInputDirty = true;
    }
    // Reason: brake dominant mode
    else if (TargetBrake > ConflictThreshold && TargetThrottle < ConflictThreshold)
    {
        PendingInput.Throttle = 0.0f;
        PendingInput.Brake = FMath::FInterpTo(CurrentBrake, TargetBrake, DeltaTime, BrakeRate);
        bInputDirty = true;
    }
    // Reason: both active - brake priority
    else if (TargetThrottle > ConflictThreshold && TargetBrake > ConflictThreshold)
    {
        PendingInput.Throttle = 0.0f;
        PendingInput.Brake = FMath::FInterpTo(CurrentBrake, TargetBrake, DeltaTime, BrakeRate);
        bInputDirty = true;
    }
    // Reason: both released - coast to zero
    else
    {
        PendingInput.Throttle = FMath::FInterpTo(CurrentThrottle, 0.0f, DeltaTime, ThrottleRate);
        PendingInput.Brake = FMath::FInterpTo(CurrentBrake, 0.0f, DeltaTime, BrakeRate);
        bInputDirty = true;
    } // End if (throttle/brake conflict resolution)

    PendingInput.Steering = FMath::FInterpTo(CurrentSteering, TargetSteering, DeltaTime, SteeringRate);
    PendingInput.Handbrake = FMath::FInterpTo(CurrentHandbrake, TargetHandbrake, DeltaTime, HandbrakeRate);
}

void AVehicleController::DisplayInputDiagnostics() const
{
    // Reserved for future implementation
}