#include "VehicleController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "VehicleSolver.h"
#include "PlayerTracker.h"
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
    bool bIsServer;                  // [-] - Is this the server?
    bool bIsLocalController;         // [-] - Is this the local player?
    FString VehicleName;             // [-] - Possessed vehicle name
    bool bVehicleHasOwner;           // [-] - Does vehicle have owner set?
    bool bVehicleHasNetConnection;   // [-] - Does vehicle have net connection?
    ENetRole LocalRole;              // [-] - Actor's local role
    ENetRole RemoteRole;             // [-] - Actor's remote role
    float Ping;                      // [ms] - Network ping
    float Throttle;                  // [0..1] - Throttle input
    float Brake;                     // [0..1] - Brake input
    float Steering;                  // [-1..1] - Steering input
    FVector Location;                // [cm] - Vehicle location
    FVector Velocity;                // [cm/s] - Vehicle velocity
    float Speed;                     // [km/h] - Vehicle speed
    int32 PacketsSent;               // [-] - Network packets sent
    int32 PacketsReceived;           // [-] - Network packets received
    float BytesSentPerSec;           // [B/s] - Network bandwidth out
    float BytesReceivedPerSec;       // [B/s] - Network bandwidth in
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
        UE_LOG(LogVehicleController, Warning, TEXT("   Save Location: %s"), *FPaths::Combine(FPaths::ProjectDir(), TEXT("Saved"), TEXT("Telemetry")));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
                FString::Printf(TEXT("📊 Telemetry: %s | Buffer: %d samples"), 
                    GbTelemetryEnabled ? TEXT("ENABLED") : TEXT("DISABLED"), 
                    GTelemetryBuffer.Num()));
        }
    })
);

static FAutoConsoleCommand TelemetryToggleCmd(
    TEXT("GRIT.Telemetry.Toggle"),
    TEXT("Toggle telemetry on/off"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        GbTelemetryEnabled = !GbTelemetryEnabled;
        UE_LOG(LogVehicleController, Warning, TEXT("📊 Telemetry %s"), GbTelemetryEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, GbTelemetryEnabled ? FColor::Green : FColor::Red, 
                FString::Printf(TEXT("📊 Telemetry %s"), GbTelemetryEnabled ? TEXT("ENABLED") : TEXT("DISABLED")));
        }
    })
);

//------------------------------------------------------------------------------
//                              constructor
//------------------------------------------------------------------------------
AVehicleController::AVehicleController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    UE_LOG(LogVehicleController, Log, TEXT("🎮 Controller bootstrapped | %s"), *GetName());
}

//------------------------------------------------------------------------------
//                              begin play
//------------------------------------------------------------------------------
void AVehicleController::BeginPlay()
{
    Super::BeginPlay();

    EstablishVehicleConnection();

    if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            if (VehicleInputMappingContext) // Reason: guard null mapping context
            {
                Subsystem->AddMappingContext(VehicleInputMappingContext, 0);
                UE_LOG(LogVehicleController, Log, TEXT("🕹️ Mapping context injected | %s"), *GetName());
            }
            else
            {
                UE_LOG(LogVehicleController, Warning, TEXT("⚠️ Mapping context null | %s"), *GetName());
            } // End if (mapping context check)
        }
    }
}

//------------------------------------------------------------------------------
//                              tick
//------------------------------------------------------------------------------
void AVehicleController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAnalogInputs(DeltaTime);

    //------------------------------------------------------------------------------
    // route inputs based on network role
    //------------------------------------------------------------------------------
    if (ControlledVehicle)
    {
        if (IsLocalController() && !HasAuthority()) // Reason: local client sends RPCs
        {
            if (!PendingInput.NearlyEquals(LastSentInput, 1.e-3f)) // Reason: delta compression
            {
                ServerSendInput(PendingInput);
                LastSentInput = PendingInput;
            } // End if (input changed)
        } // End if (client authority)
        else if (HasAuthority()) // Reason: server applies directly
        {
            ControlledVehicle->InputTensor_GameThread = PendingInput;
        } // End if (server authority)
    } // End if (vehicle valid)

    //------------------------------------------------------------------------------
    // capture multiplayer telemetry every frame
    //------------------------------------------------------------------------------
    if (GbTelemetryEnabled && ControlledVehicle)
    {
        FMultiplayerTelemetry Sample;
        Sample.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        Sample.PlayerName = GetName();
        Sample.bIsServer = GetLocalRole() == ROLE_Authority;
        Sample.bIsLocalController = IsLocalController();
        Sample.VehicleName = ControlledVehicle->GetName();
        Sample.bVehicleHasOwner = (ControlledVehicle->GetOwner() != nullptr);
        Sample.bVehicleHasNetConnection = (ControlledVehicle->GetNetConnection() != nullptr);
        Sample.LocalRole = ControlledVehicle->GetLocalRole();
        Sample.RemoteRole = ControlledVehicle->GetRemoteRole();
        Sample.Ping = PlayerState ? PlayerState->GetPingInMilliseconds() : 0.0f; // [ms]
        Sample.Throttle = ControlledVehicle->InputTensor_GameThread.Throttle;
        Sample.Brake = ControlledVehicle->InputTensor_GameThread.Brake;
        Sample.Steering = ControlledVehicle->InputTensor_GameThread.Steering;
        
        if (UStaticMeshComponent* Hull = ControlledVehicle->VehicleHull)
        {
            Sample.Location = Hull->GetComponentLocation();
            Sample.Velocity = Hull->GetPhysicsLinearVelocity();
            Sample.Speed = Sample.Velocity.Size() * 0.036f; // [km/h]
        }
        else
        {
            Sample.Location = FVector::ZeroVector;
            Sample.Velocity = FVector::ZeroVector;
            Sample.Speed = 0.0f;
        } // End if (hull exists)

        if (UNetConnection* NetConn = GetNetConnection())
        {
            Sample.PacketsSent = NetConn->OutPackets;
            Sample.PacketsReceived = NetConn->InPackets;
            Sample.BytesSentPerSec = NetConn->OutBytesPerSecond;
            Sample.BytesReceivedPerSec = NetConn->InBytesPerSecond;
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
        
        // Debug log every 60 frames (1 second at 60 FPS) to show telemetry is working
        static int32 TelemetryFrameCount = 0;
        if (++TelemetryFrameCount % 60 == 0)
        {
            UE_LOG(LogVehicleController, Log, TEXT("📊 Telemetry capturing | Player: %s | Samples: %d | Speed: %.1f km/h | Owner: %s | Role: %s"), 
                *Sample.PlayerName, 
                GTelemetryBuffer.Num(), 
                Sample.Speed,
                Sample.bVehicleHasOwner ? TEXT("YES") : TEXT("NO"),
                Sample.bIsServer ? TEXT("SERVER") : TEXT("CLIENT"));
        }
    } // End if (telemetry enabled)
}

//------------------------------------------------------------------------------
//                              setup input
//------------------------------------------------------------------------------
void AVehicleController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput) // Reason: ensure enhanced input availability
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

//------------------------------------------------------------------------------
//                              on possess - CRITICAL FIX
//------------------------------------------------------------------------------
void AVehicleController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledVehicle = Cast<AVehicleSolver>(InPawn);
    if (ControlledVehicle) // Reason: confirm vehicle possession
    {
        //------------------------------------------------------------------------------
        // CRITICAL: set vehicle owner to this controller for RPC routing
        //------------------------------------------------------------------------------
        ControlledVehicle->SetOwner(this);
        
        //------------------------------------------------------------------------------
        // CRITICAL: force immediate network update to replicate owner to clients
        //------------------------------------------------------------------------------
        if (GetLocalRole() == ROLE_Authority) // Reason: only server can force replication
        {
            ControlledVehicle->ForceNetUpdate();
        } // End if (server authority)
        
        bConnectionEstablished = true;
        PendingInput.FlushInputs();
        LastSentInput.FlushInputs();
        bInputDirty = false;
        
        const bool bIsServer = (GetLocalRole() == ROLE_Authority);
        const bool bHasOwner = (ControlledVehicle->GetOwner() != nullptr);
        
        UE_LOG(LogVehicleController, Log, TEXT("✅ Vehicle possession established | Controller: %s | Vehicle: %s | Role: %s | Owner: %s"), 
            *GetName(), 
            *ControlledVehicle->GetName(), 
            bIsServer ? TEXT("SERVER") : TEXT("CLIENT"), 
            bHasOwner ? *ControlledVehicle->GetOwner()->GetName() : TEXT("NONE"));
    }
    else
    {
        bConnectionEstablished = false;
        UE_LOG(LogVehicleController, Warning, TEXT("⚠️ Pawn not a VehicleConstruct | Controller: %s"), *GetName());
    } // End if (possession validation)
}

//------------------------------------------------------------------------------
//                              on unpossess
//------------------------------------------------------------------------------
void AVehicleController::OnUnPossess()
{
    //------------------------------------------------------------------------------
    // write telemetry to CSV before unpossessing
    //------------------------------------------------------------------------------
    if (GbTelemetryEnabled && GTelemetryBuffer.Num() > 0)
    {
        FString ProjectDir = FPaths::ProjectDir();
        FString TelemetryDir = FPaths::Combine(ProjectDir, TEXT("Saved"), TEXT("Telemetry"));
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        
        if (!PlatformFile.DirectoryExists(*TelemetryDir)) // Reason: ensure directory exists
        {
            PlatformFile.CreateDirectory(*TelemetryDir);
        } // End if (directory creation)

        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        FString PlayerID = GetName().Replace(TEXT("VehicleController"), TEXT("Player"));
        FString Filename = FString::Printf(TEXT("Multiplayer_%s_%s.csv"), *PlayerID, *Timestamp);
        FString FilePath = FPaths::Combine(TelemetryDir, Filename);

        FString CsvContent = TEXT("Timestamp,PlayerName,IsServer,IsLocalController,VehicleName,HasOwner,HasNetConnection,LocalRole,RemoteRole,Ping_ms,Throttle,Brake,Steering,LocationX,LocationY,LocationZ,VelX,VelY,VelZ,Speed_kmh,PacketsSent,PacketsReceived,BytesSent,BytesReceived\n");

        FScopeLock Lock(&GTelemetryMutex);
        for (const FMultiplayerTelemetry& Sample : GTelemetryBuffer)
        {
            FString RoleStr = (Sample.LocalRole == ROLE_Authority) ? TEXT("Authority") : (Sample.LocalRole == ROLE_AutonomousProxy) ? TEXT("AutonomousProxy") : TEXT("SimulatedProxy");
            FString RemoteRoleStr = (Sample.RemoteRole == ROLE_Authority) ? TEXT("Authority") : (Sample.RemoteRole == ROLE_AutonomousProxy) ? TEXT("AutonomousProxy") : TEXT("SimulatedProxy");
            
            CsvContent += FString::Printf(TEXT("%.3f,%s,%d,%d,%s,%d,%d,%s,%s,%.1f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%d,%d,%.1f,%.1f\n"), Sample.Timestamp, *Sample.PlayerName, Sample.bIsServer ? 1 : 0, Sample.bIsLocalController ? 1 : 0, *Sample.VehicleName, Sample.bVehicleHasOwner ? 1 : 0, Sample.bVehicleHasNetConnection ? 1 : 0, *RoleStr, *RemoteRoleStr, Sample.Ping, Sample.Throttle, Sample.Brake, Sample.Steering, Sample.Location.X, Sample.Location.Y, Sample.Location.Z, Sample.Velocity.X, Sample.Velocity.Y, Sample.Velocity.Z, Sample.Speed, Sample.PacketsSent, Sample.PacketsReceived, Sample.BytesSentPerSec, Sample.BytesReceivedPerSec);
        } // End for (each sample)

        if (FFileHelper::SaveStringToFile(CsvContent, *FilePath))
        {
            UE_LOG(LogVehicleController, Warning, TEXT("📊 MULTIPLAYER TELEMETRY SAVED | File: %s | Samples: %d | Location: %s"), *FilePath, GTelemetryBuffer.Num(), *TelemetryDir);
            
            // Also log to screen for visibility
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, 
                    FString::Printf(TEXT("📊 Telemetry saved: %s (%d samples)"), *Filename, GTelemetryBuffer.Num()));
            }
        }
        else
        {
            UE_LOG(LogVehicleController, Error, TEXT("❌ FAILED TO WRITE TELEMETRY | File: %s"), *FilePath);
            
            // Also log to screen for visibility
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, 
                    FString::Printf(TEXT("❌ Failed to save telemetry: %s"), *Filename));
            }
        } // End if (file write)

        GTelemetryBuffer.Empty();
    } // End if (telemetry write)

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
    }
}

//------------------------------------------------------------------------------
//                              server RPC
//------------------------------------------------------------------------------
void AVehicleController::ServerSendInput_Implementation(FInputTensor NewInput)
{
    if (!ControlledVehicle) // Reason: require valid vehicle reference
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

//------------------------------------------------------------------------------
//                              input handlers
//------------------------------------------------------------------------------
void AVehicleController::ThrottleTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    const float InputValue = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
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

    const float InputValue = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
    TargetBrake = InputValue;
}

void AVehicleController::BrakeCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return;

    TargetBrake = 0.0f;
}

void AVehicleController::SteerTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetSteering = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void AVehicleController::SteerCompleted(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetSteering = 0.0f;
}

void AVehicleController::HandbrakeTriggered(const FInputActionValue& Value)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    TargetHandbrake = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
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

    PendingInput.Clutch = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
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

//------------------------------------------------------------------------------
//                              establish connection
//------------------------------------------------------------------------------
void AVehicleController::EstablishVehicleConnection()
{
    bConnectionEstablished = false;
}

//------------------------------------------------------------------------------
//                              update analog inputs
//------------------------------------------------------------------------------
void AVehicleController::UpdateAnalogInputs(float DeltaTime)
{
    if (!ControlledVehicle) return; // Reason: require vehicle link

    const float CurrentThrottle = PendingInput.Throttle;
    const float CurrentBrake = PendingInput.Brake;
    const float CurrentSteering = PendingInput.Steering;
    const float CurrentHandbrake = PendingInput.Handbrake;

    //------------------------------------------------------------------------------
    // input smoothing with conflict resolution
    //------------------------------------------------------------------------------
    constexpr float ConflictThreshold = 0.05f; // [-] - Minimum input to trigger conflict resolution
    
    if (TargetThrottle > ConflictThreshold && TargetBrake < ConflictThreshold) // Reason: throttle dominant
    {
        PendingInput.Brake = 0.0f;
        PendingInput.Throttle = FMath::FInterpTo(CurrentThrottle, TargetThrottle, DeltaTime, ThrottleRate);
        bInputDirty = true;
    }
    else if (TargetBrake > ConflictThreshold && TargetThrottle < ConflictThreshold) // Reason: brake dominant
    {
        PendingInput.Throttle = 0.0f;
        PendingInput.Brake = FMath::FInterpTo(CurrentBrake, TargetBrake, DeltaTime, BrakeRate);
        bInputDirty = true;
    }
    else if (TargetThrottle > ConflictThreshold && TargetBrake > ConflictThreshold) // Reason: both active - brake wins
    {
        PendingInput.Throttle = 0.0f;
        PendingInput.Brake = FMath::FInterpTo(CurrentBrake, TargetBrake, DeltaTime, BrakeRate);
        bInputDirty = true;
    }
    else // Reason: both released - coast
    {
        PendingInput.Throttle = FMath::FInterpTo(CurrentThrottle, 0.0f, DeltaTime, ThrottleRate);
        PendingInput.Brake = FMath::FInterpTo(CurrentBrake, 0.0f, DeltaTime, BrakeRate);
        bInputDirty = true;
    } // End if (throttle/brake conflict resolution)

    PendingInput.Steering = FMath::FInterpTo(CurrentSteering, TargetSteering, DeltaTime, SteeringRate);
    PendingInput.Handbrake = FMath::FInterpTo(CurrentHandbrake, TargetHandbrake, DeltaTime, HandbrakeRate);
} // End UpdateAnalogInputs()

//------------------------------------------------------------------------------
//                              display diagnostics
//------------------------------------------------------------------------------
void AVehicleController::DisplayInputDiagnostics() const
{
    // Reserved for future implementation
}
