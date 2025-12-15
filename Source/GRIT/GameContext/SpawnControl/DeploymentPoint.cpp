#include "DeploymentPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Logging/LogMacros.h"

//------------------------------------------------------------------------------
//                                      logging
//------------------------------------------------------------------------------
DEFINE_LOG_CATEGORY(LogDeploymentPoint);

//------------------------------------------------------------------------------
//                                 static variables
//------------------------------------------------------------------------------
int32 ADeploymentPoint::GlobalDeploymentIndex = 0;
TWeakObjectPtr<UWorld> ADeploymentPoint::CachedWorld = nullptr;

//------------------------------------------------------------------------------
//                                   constructor
//------------------------------------------------------------------------------
ADeploymentPoint::ADeploymentPoint()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    RootComponent = SceneComponent;

    ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
    ArrowComponent->SetupAttachment(SceneComponent);
    ArrowComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
    BillboardComponent->SetupAttachment(ArrowComponent);
    BillboardComponent->SetHiddenInGame(true);

    DeploymentIndex = 0;

    UE_LOG(LogDeploymentPoint, Log, TEXT("📍 Spawn scaffold initialized | Actor: %s"), *GetName());
}

ADeploymentPoint::~ADeploymentPoint()
{
}

void ADeploymentPoint::BeginDestroy()
{
    Super::BeginDestroy();
    if (bHasAssignedDeploymentIndex)
    {
        GlobalDeploymentIndex = FMath::Max(0, GlobalDeploymentIndex - 1);
        bHasAssignedDeploymentIndex = false;
        UE_LOG(LogDeploymentPoint, Log, TEXT("🏴 Index retired | Actor: %s | RemainingCount: %d"), *GetName(), GlobalDeploymentIndex);
    }
}

void ADeploymentPoint::BeginPlay()
{
    Super::BeginPlay();

    UWorld* CurrentWorld = GetWorld();
    if (CachedWorld.Get() != CurrentWorld) // Reason: new world detected requiring counter reset
    {
        CachedWorld = CurrentWorld;
        GlobalDeploymentIndex = 0;
        UE_LOG(LogDeploymentPoint, Warning, TEXT("⚠️ Counter reset due to world transition | World: %s"), CurrentWorld ? *CurrentWorld->GetName() : TEXT("<null>"));
    } // End if (world switch)

    if (!bHasAssignedDeploymentIndex) // Reason: ensure index assigned exactly once per actor
    {
        DeploymentIndex = GlobalDeploymentIndex++;
        bHasAssignedDeploymentIndex = true;
        UE_LOG(LogDeploymentPoint, Log, TEXT("🏁 Index armed | Actor: %s | Index: %d"), *GetName(), DeploymentIndex);
    } // End if (index assignment gate)

    if (bCanSelfDestruct) // Reason: self-destruction requested via editor flag
    {
        UE_LOG(LogDeploymentPoint, Log, TEXT("🗑️ Auto-destruct engaged | Actor: %s"), *GetName());
        Destroy();
    } // End if (self-destruct trigger)
}

void ADeploymentPoint::PostLoad()
{
    Super::PostLoad();
    UE_LOG(LogDeploymentPoint, Verbose, TEXT("🏳️ Post-load checkpoint | Actor: %s"), *GetName());
}

//------------------------------------------------------------------------------
//                            editor creation monitoring
//------------------------------------------------------------------------------
void ADeploymentPoint::PostActorCreated()
{
    Super::PostActorCreated();

    const UWorld* PlacementWorld = GetWorld();
    const FString WorldName = PlacementWorld ? PlacementWorld->GetName() : TEXT("<null>");
    const FString LocationString = GetActorLocation().ToCompactString();

    UE_LOG(LogDeploymentPoint, Log, TEXT("🚩 Editor placement detected | ActorClass: %s | Location: %s | World: %s | WorldPtr: %p"), *GetClass()->GetName(),  *LocationString, *WorldName, PlacementWorld);
}

void ADeploymentPoint::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (BillboardComponent) // Reason: ensure target component exists before adjusting offset
    {
        const FVector CurrentRelative = BillboardComponent->GetRelativeLocation();
        BillboardComponent->SetRelativeLocation(FVector(CurrentRelative.X, CurrentRelative.Y, Offset));
    } // End if (billboard valid)
}
