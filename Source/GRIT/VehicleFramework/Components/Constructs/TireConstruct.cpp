// TireConstruct.cpp
#include "TireConstruct.h"
#include "VehicleFramework/VehicleSolver.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

ATireConstruct::ATireConstruct()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    TireGeometry = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TireGeometry"));
    RootComponent = TireGeometry;

    MagsMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagsMesh"));
    MagsMesh->SetupAttachment(TireGeometry);
}

void ATireConstruct::BeginPlay()
{
    Super::BeginPlay();

    // Warn if author forgot the InnerRadius socket — DeriveGeometryFromMesh will fall back.
    if (TireGeometry && TireGeometry->GetStaticMesh()
        && !TireGeometry->DoesSocketExist(TireConstruct_InnerRadiusSocket))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ATireConstruct '%s': mesh has no '%s' socket; inner radius will fall back to 0.55*outer."),
            *GetName(), *TireConstruct_InnerRadiusSocket.ToString());
    }

    // Sync radii from mesh at runtime so SpecSheet reflects the assigned geometry
    // before any equip/query. If no mesh is assigned, defaults are preserved.
    RefreshSpecFromMesh();
}

void ATireConstruct::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Keep radii in sync in the editor and on Construction Script runs so placed
    // instances show correct values without needing to PIE or equip.
    RefreshSpecFromMesh();
}

#if WITH_EDITOR
void ATireConstruct::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RefreshSpecFromMesh();
}
#endif

void ATireConstruct::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATireConstruct, SpecSheet);
}

void ATireConstruct::DeriveGeometryFromMesh(float& OutOuterRadius_m, float& OutInnerRadius_m, float& OutWidth_m) const
{
    OutOuterRadius_m = SpecSheet.OuterRadius;
    OutInnerRadius_m = SpecSheet.InnerRadius;
    OutWidth_m = SpecSheet.Width;

    if (!TireGeometry || !TireGeometry->GetStaticMesh()) { return; }

    // Local-space bounds extent (UE units = cm).
    const FBoxSphereBounds Bounds = TireGeometry->GetStaticMesh()->GetBounds();
    const FVector Ext = Bounds.BoxExtent;

    constexpr float CmToM = 0.01f;
    const float DiameterCm = FMath::Max(Ext.X, Ext.Z) * 2.0f;
    OutOuterRadius_m = (DiameterCm * 0.5f) * CmToM;
    OutWidth_m = (Ext.Y * 2.0f) * CmToM;

    if (TireGeometry->DoesSocketExist(TireConstruct_InnerRadiusSocket))
    {
        // Socket transform in component (mesh-local) space, projected into X/Z plane.
        const FTransform SockLocal = TireGeometry->GetSocketTransform(
            TireConstruct_InnerRadiusSocket, ERelativeTransformSpace::RTS_Component);
        const FVector P = SockLocal.GetLocation();
        const float InnerCm = FMath::Sqrt(P.X * P.X + P.Z * P.Z);
        OutInnerRadius_m = InnerCm * CmToM;
    }
    else
    {
        OutInnerRadius_m = OutOuterRadius_m * 0.55f;
    }
}

void ATireConstruct::RefreshSpecFromMesh()
{
    float OuterM = SpecSheet.OuterRadius;
    float InnerM = SpecSheet.InnerRadius;
    float WidthM = SpecSheet.Width;
    DeriveGeometryFromMesh(OuterM, InnerM, WidthM);

    SpecSheet.OuterRadius = OuterM;
    SpecSheet.InnerRadius = InnerM;
    SpecSheet.Width = WidthM;

    if (TireGeometry && TireGeometry->GetStaticMesh())
    {
        SpecSheet.TireMesh = TireGeometry->GetStaticMesh();
    }
    if (MagsMesh && MagsMesh->GetStaticMesh())
    {
        SpecSheet.MagsMesh = MagsMesh->GetStaticMesh();
    }
    if (TireGeometry)
    {
        if (UMaterialInterface* Mat = TireGeometry->GetMaterial(0))
        {
            SpecSheet.BaseRubberMaterial = Mat;
        }
    }
}

bool ATireConstruct::TryEquipTo(AVehicleSolver* Solver, int32 WheelSlot)
{
    if (!Solver) { return false; }
    RefreshSpecFromMesh();
    Solver->Server_EquipTire(WheelSlot, this);
    return true;
}
