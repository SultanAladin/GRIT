#include "VehicleConstruct.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"


//------------------------------------------------------------------------------
//                                   constructor
//------------------------------------------------------------------------------
AVehicleConstruct::AVehicleConstruct()
{
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(VehicleHull);
    SpringArm->TargetArmLength = 750.0f;               // [cm] - Camera offset behind vehicle
    SpringArm->SocketOffset = FVector(0.0f, 0.0f, 250.0f); // [cm] - Raise camera above vehicle
    SpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
    SpringArm->bUsePawnControlRotation = false;        // reason: follow vehicle rotation instead of controller
    SpringArm->bInheritPitch = true;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = true;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 12.0f;                 // [s⁻¹] - Lag smoothing factor
    SpringArm->bEnableCameraRotationLag = true;
    SpringArm->CameraRotationLagSpeed = 8.0f;          // [s⁻¹] - smooths rotation follow

    ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
    ChaseCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    ChaseCamera->bUsePawnControlRotation = false;
}

//------------------------------------------------------------------------------
//                                    begin play
//------------------------------------------------------------------------------
void AVehicleConstruct::BeginPlay()
{
    Super::BeginPlay();

    if (ChaseCamera) // Reason: ensure camera exists before adjusting FOV
    {
        ChaseCamera->SetFieldOfView(90.0f);
    } // End if (camera availability)


}

void AVehicleConstruct::EndPlay(const EEndPlayReason::Type EndPlayReason)
{


    Super::EndPlay(EndPlayReason);
}
