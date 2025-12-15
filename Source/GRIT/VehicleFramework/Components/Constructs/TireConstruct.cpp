// TireConstruct.cpp
#include "TireConstruct.h"
#include "Net/UnrealNetwork.h"

ATireConstruct::ATireConstruct()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    TireGeometry = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TireGeometry"));
    RootComponent = TireGeometry;

    SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
    SkeletalMesh->SetupAttachment(RootComponent);
    SkeletalMesh->SetVisibility(false);
}

void ATireConstruct::BeginPlay()
{
    Super::BeginPlay();
}

void ATireConstruct::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ATireConstruct::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATireConstruct, SpecSheet);
}

void ATireConstruct::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}