#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "ViewportNavigator.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AVehicleConstruct;

//------------------------------------------------------------------------------
//                          CAMERA MODES
//------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EViewportMode : uint8
{
    Orbital     UMETA(DisplayName = "Orbital View"),
    Cinematic   UMETA(DisplayName = "Cinematic"),
    Snapshot    UMETA(DisplayName = "Snapshot View"),
    Static      UMETA(DisplayName = "Static View")
};

//------------------------------------------------------------------------------
//                          VIEWPORT NAVIGATOR CONTROLLER
//------------------------------------------------------------------------------
/** Camera controller for configurator viewport - orbital camera around stationary vehicle */
UCLASS(Blueprintable)
class GRIT_API AViewportNavigator : public APlayerController
{
    GENERATED_BODY()

public:
    AViewportNavigator();

    /** Transition camera to anchor transform (socket position) */
    UFUNCTION(BlueprintCallable, Category = "Camera Control")
    void TransitionToAnchor(const FTransform& TargetTransform);

    /** Get spring arm component */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    USpringArmComponent* GetSpringArm() const { return Arm; }

    /** Get camera component */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    UCameraComponent* GetCamera() const { return Cam; }

    /** Lock vehicle brakes (prevents rolling on slopes) */
    UFUNCTION(BlueprintCallable, Category = "Vehicle Control")
    void SetBrakeLock(bool bLocked);

    /** Check if brakes are locked */
    UFUNCTION(BlueprintPure, Category = "Vehicle Control")
    bool IsBrakeLocked() const { return bBrakesLocked; }

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

private:
    //--------------------------------------------------------------------------
    //                          ANCHOR TRANSITION
    //--------------------------------------------------------------------------
    void UpdateAnchorTransition(float DeltaTime);

    //--------------------------------------------------------------------------
    //                          INPUT PROCESSING
    //--------------------------------------------------------------------------
    void ProcessCameraInput(const FInputActionValue& Value);
    void ProcessZoomInput(const FInputActionValue& Value);
    void ProcessMiddleMousePressed(const FInputActionValue& Value);
    void ProcessMiddleMouseReleased(const FInputActionValue& Value);
    void ProcessShiftPressed(const FInputActionValue& Value);
    void ProcessShiftReleased(const FInputActionValue& Value);
    void ProcessToggleMouse(const FInputActionValue& Value);
    void ProcessCycleMode(const FInputActionValue& Value);

    //--------------------------------------------------------------------------
    //                          MOVEMENT LOGIC
    //--------------------------------------------------------------------------
    void ProcessOrbitalAxisInput(const FVector2D& MovementVector);
    void ProcessPanInput(const FVector2D& MovementVector);

    //--------------------------------------------------------------------------
    //                          CAMERA UPDATE
    //--------------------------------------------------------------------------
    void UpdateCameraOrbiting(float DeltaTime);
    void UpdateCameraZooming(float DeltaTime);
    void UpdateCameraPanning(float DeltaTime);

    //--------------------------------------------------------------------------
    //                          UTILITY
    //--------------------------------------------------------------------------
    bool EnsureRequiredComponentsExist();
    float GetTraceDistance(UCameraComponent* Camera, FVector Direction, float MaxDistance);
    float RestrictZoomByDistance(float CurrentLength, float DesiredLength, float Distance);
    void UpdateDistanceMeasurements();
    void InitializeCamera();
    bool CacheVehicleComponents();
    FVector GetPanPivotLocation() const;

public:
    //--------------------------------------------------------------------------
    //                          INPUT ASSETS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputMappingContext> CameraMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> OrbitalAxisControl;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> ZoomAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> MiddleMouseAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> PanEnableAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> CycleModeAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> ToggleMouseCursorAction;

    //--------------------------------------------------------------------------
    //                          CAMERA MODE
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode")
    EViewportMode CurrentMode = EViewportMode::Orbital;

    //--------------------------------------------------------------------------
    //                          SENSITIVITY SETTINGS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Sensitivity")
    float OrbitalSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Sensitivity")
    float ZoomSpeed = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Sensitivity")
    float PanSensitivity = 1.0f;

    //--------------------------------------------------------------------------
    //                          SMOOTHING SETTINGS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Smoothing")
    float CameraLagSpeed = 5.0f;

    //--------------------------------------------------------------------------
    //                          ORBITAL SETTINGS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Orbital")
    float MinPitchAngle = -80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Orbital")
    float MaxPitchAngle = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Orbital")
    float TargetYaw = 0.0f;                               // [deg] - Initial yaw angle

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Orbital")
    float TargetPitch = -10.0f;                           // [deg] - Initial pitch angle

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Orbital")
    float OrbitalSafetyDistance = 50.0f;                  // [cm] - Ground avoidance buffer

    //--------------------------------------------------------------------------
    //                          ZOOM SETTINGS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Zoom")
    float MinZoomDistance = 100.0f;                       // [cm]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Zoom")
    float MaxZoomDistance = 1500.0f;                      // [cm]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Zoom")
    float TargetArmLength = 750.0f;                       // [cm] - Initial arm length

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Zoom")
    float DefaultArmLength = 1000.0f;                     // [cm]

    //--------------------------------------------------------------------------
    //                          PAN SETTINGS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "Camera Settings|Pan")
    float MinPanZ = -200.0f;                              // [cm]

    UPROPERTY(EditAnywhere, Category = "Camera Settings|Pan")
    float MaxPanZ = 800.0f;                               // [cm]

    //--------------------------------------------------------------------------
    //                          COLLISION SETTINGS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "Camera Settings|Collision")
    float CollisionSafetyThreshold = 50.0f;               // [cm]

    UPROPERTY(EditAnywhere, Category = "Camera Settings|Collision")
    float ProximityMultiplier = 4.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Settings|Collision")
    float ProximityBuffer = 30.0f;                        // [cm]

    //--------------------------------------------------------------------------
    //                          ANCHOR TRANSITION SETTINGS
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Anchor")
    float AnchorTransitionSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings|Anchor")
    float AnchorRotationSpeed = 1.5f;

    //--------------------------------------------------------------------------
    //                          CAMERA STATE (READ-ONLY)
    //--------------------------------------------------------------------------
    UPROPERTY(BlueprintReadOnly, Category = "Camera State")
    float CurrentYaw = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Camera State")
    float CurrentPitch = -10.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Camera State")
    FVector2D TargetPanLocation;

    UPROPERTY(BlueprintReadOnly, Category = "Camera State")
    float TargetZPosition = 0.0f;

    //--------------------------------------------------------------------------
    //                          INPUT STATE (READ-ONLY)
    //--------------------------------------------------------------------------
    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bIsMiddleMousePressed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bIsShiftPressed = false;

    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    bool bIsPanning = false;

    UPROPERTY(BlueprintReadOnly, Category = "Input State")
    FVector2D CurrentPanInput;

private:
    //--------------------------------------------------------------------------
    //                          RUNTIME STATE
    //--------------------------------------------------------------------------
    AVehicleConstruct* Vehicle = nullptr;
    USpringArmComponent* Arm = nullptr;
    UCameraComponent* Cam = nullptr;

    FVector LastPanLoc = FVector::ZeroVector;
    bool bFirstRun = true;
    bool bBrakesLocked = false;                           // [-] - Brake lock state

    //--------------------------------------------------------------------------
    //                          DISTANCE TRACKING
    //--------------------------------------------------------------------------
    float ForwardDistance = 0.0f;
    float RightDistance = 0.0f;
    float UpDistance = 0.0f;
    float LeftDistance = 0.0f;
    float DownDistance = 0.0f;
    float InverseForwardDistance = 0.0f;

    //--------------------------------------------------------------------------
    //                          ANCHOR TRANSITION STATE
    //--------------------------------------------------------------------------
    bool bIsTransitioningToAnchor = false;
    float AnchorTransitionAlpha = 0.0f;
    FVector AnchorStartLocation;
    FRotator AnchorStartRotation;
    float AnchorStartArmLength = 0.0f;
    FVector TargetAnchorLocation;
    FRotator TargetAnchorRotation;
};
