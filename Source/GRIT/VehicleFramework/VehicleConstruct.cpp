#include "VehicleConstruct.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"


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

    // Spawn HMI widgets at configured socket locations
    SpawnHMIWidgets();
}

void AVehicleConstruct::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up HMI widgets
    ClearHMIWidgets();

    Super::EndPlay(EndPlayReason);
}

//------------------------------------------------------------------------------
//                                HMI WIDGET SYSTEM
//------------------------------------------------------------------------------

void AVehicleConstruct::SpawnHMIWidgets()
{
    UStaticMeshComponent* MeshComp = VehicleHull;
    if (!MeshComp) // Reason: socket attachment requires mesh
    {
        UE_LOG(LogTemp, Warning, TEXT("VehicleConstruct: Cannot spawn HMI widgets - no vehicle hull component"));
        return;
    } // End if (mesh validity)

    UE_LOG(LogTemp, Log, TEXT("VehicleConstruct: Spawning %d HMI widgets"), HMIMenus.Num());

    for (const FVehicleMenuEntry& Entry : HMIMenus)
    {
        if (!Entry.WidgetClass || Entry.AttachSocket.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("VehicleConstruct: Skipping invalid HMI entry - WidgetClass=%s, Socket=%s"), 
                Entry.WidgetClass ? TEXT("Valid") : TEXT("NULL"), 
                *Entry.AttachSocket.ToString());
            continue; // Reason: invalid configuration
        } // End if (entry validity)

        // Check if socket exists on mesh
        if (!MeshComp->DoesSocketExist(Entry.AttachSocket))
        {
            UE_LOG(LogTemp, Warning, TEXT("VehicleConstruct: Socket '%s' does not exist on mesh"), *Entry.AttachSocket.ToString());
            continue;
        } // End if (socket validity)

        // Create widget component
        UWidgetComponent* WidgetComp = NewObject<UWidgetComponent>(this);
        if (!WidgetComp)
        {
            UE_LOG(LogTemp, Error, TEXT("VehicleConstruct: Failed to create widget component for socket '%s'"), *Entry.AttachSocket.ToString());
            continue;
        } // End if (creation failed)

        // Attach to socket with proper transform inheritance
        WidgetComp->SetupAttachment(MeshComp, Entry.AttachSocket);
        WidgetComp->RegisterComponent();

        // Configure widget component properties
        WidgetComp->SetWidgetSpace(Entry.WidgetSpace); // Reason: HMI must exist in world space
        WidgetComp->SetWidgetClass(Entry.WidgetClass);
        WidgetComp->SetDrawAtDesiredSize(Entry.bDrawAtDesiredSize);
        WidgetComp->SetTwoSided(Entry.bTwoSided);

        // Reason: enable mouse interaction with world-space widget
        if (Entry.bEnableInteraction)
        {
            WidgetComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            WidgetComp->SetCollisionResponseToAllChannels(ECR_Ignore);
            WidgetComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Reason: required for mouse trace hits
            WidgetComp->SetGenerateOverlapEvents(false);
            WidgetComp->SetWindowFocusable(true);
        } // End if (interaction enabled)

        // Reason: inherit socket transform exactly, apply optional offset with scale
        FTransform FinalTransform = Entry.RelativeOffset;
        FinalTransform.SetScale3D(FinalTransform.GetScale3D() * Entry.WidgetScale); // [-] - Apply widget content scaling
        WidgetComp->SetRelativeTransform(FinalTransform);

        // Store references for management
        SpawnedHMIComponents.Add(WidgetComp);
        SocketToWidgetMap.Add(Entry.AttachSocket, WidgetComp);

        UE_LOG(LogTemp, Log, TEXT("VehicleConstruct: Successfully spawned HMI widget at socket '%s'"), *Entry.AttachSocket.ToString());
    } // End for (each menu entry)

    UE_LOG(LogTemp, Log, TEXT("VehicleConstruct: HMI widget spawning complete - %d widgets created"), SpawnedHMIComponents.Num());
}

void AVehicleConstruct::ClearHMIWidgets()
{
    UE_LOG(LogTemp, Log, TEXT("VehicleConstruct: Clearing %d HMI widgets"), SpawnedHMIComponents.Num());

    // Destroy all spawned widget components
    for (UWidgetComponent* WidgetComp : SpawnedHMIComponents)
    {
        if (WidgetComp && IsValid(WidgetComp))
        {
            WidgetComp->DestroyComponent();
        } // End if (valid component)
    } // End for (each component)

    // Clear arrays
    SpawnedHMIComponents.Empty();
    SocketToWidgetMap.Empty();

    UE_LOG(LogTemp, Log, TEXT("VehicleConstruct: HMI widget cleanup complete"));
}

UWidgetComponent* AVehicleConstruct::GetHMIWidget(FName SocketName) const
{
    if (UWidgetComponent* const* FoundWidget = SocketToWidgetMap.Find(SocketName))
    {
        return *FoundWidget;
    } // End if (widget found)

    return nullptr;
}