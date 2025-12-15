/*====================================================================================================================================
                                                         ADAPTIVE AERODYNAMICS SYSTEM
======================================================================================================================================*/
#pragma once
#include "CoreMinimal.h"
#include "AerodynamicSpecifications.generated.h"

//------------------------------------------------------------------------------
//                                               UTILITY FUNCTIONS
//------------------------------------------------------------------------------
/**
 * Computes the force application point relative to the center of mass.
 * This is used by all aero systems to convert socket world locations to a single
 * COM-relative force application point for physics thread use.
 * 
 * @param SocketWorldLocations - Array of world space socket positions
 * @param CenterOfMass - World space center of mass position
 * @return FVector - Average socket position relative to COM (COM-relative offset)
 */
inline FVector ComputeForceApplicationPoint(const TArray<FVector>& SocketWorldLocations, const FVector& CenterOfMass)
{
    if (SocketWorldLocations.Num() == 0)
    {
        return FVector::ZeroVector;
    }
    
    FVector AverageSocketLocation = FVector::ZeroVector;
    for (const FVector& SocketLocation : SocketWorldLocations)
    {
        AverageSocketLocation += SocketLocation;
    }
    AverageSocketLocation /= SocketWorldLocations.Num();
    
    return AverageSocketLocation - CenterOfMass;
}

//------------------------------------------------------------------------------
//                                          PHYSICS THREAD STRUCTURES
//------------------------------------------------------------------------------

// --- Wing / Spoiler (Rear) - Physics Thread ---
struct FWing_PT
{
    float Area_m2;                          // [m²] Wing surface area
    float BaseCoeffLift;                    // [-] Base lift coefficient (negative for downforce)
    float BaseCoeffDrag;                    // [-] Base drag coefficient
    float AspectRatio;                      // [-] Wing aspect ratio (span²/area)
    float MaxAngle_deg;                     // [deg] Maximum wing angle
    float MinAngle_deg;                     // [deg] Minimum wing angle
    float CurrentAngle_deg;                 // [deg] Current wing angle
    bool bEnabled = false;                  // [-] Component enabled (socket exists)
    bool bIsAdaptive;                       // [-] Adaptive wing enabled
    float AdaptiveSpeedThreshold_ms;        // [m/s] Speed threshold for adaptation
    float BrakeDeployAngle_deg;             // [deg] Additional angle when braking
    FVector ForceApplicationPoint_COM;      // [m] Force application point relative to COM
};

// --- Splitter / Air Dam (Front) - Physics Thread ---
struct FSplitter_PT
{
    bool bEnabled = false;                  // [-] Component enabled (socket exists)
    float Area_m2;                          // [m²] Splitter surface area
    float CoeffPressure;                    // [-] Pressure coefficient
    float AirDamHeight_m;                   // [m] Air dam height
    float AirDamDragCoeff;                  // [-] Air dam drag coefficient
    float RideHeightSensitivity;            // [-] Ride height sensitivity factor
    FVector ForceApplicationPoint_COM;      // [m] Force application point relative to COM
};

// --- Canards (Front Wings) - Physics Thread ---
struct FCanard_PT
{
    bool bEnabled = false;                  // [-] Component enabled (socket exists)
    float Area_m2;                          // [m²] Canard surface area
    float BaseCoeffLift;                    // [-] Base lift coefficient (negative for downforce)
    float BaseCoeffDrag;                    // [-] Base drag coefficient
    float AspectRatio;                      // [-] Canard aspect ratio
    float MaxAngle_deg;                     // [deg] Maximum canard angle
    float MinAngle_deg;                     // [deg] Minimum canard angle
    float CurrentAngle_deg;                 // [deg] Current canard angle
    bool bIsAdaptive;                       // [-] Adaptive canard enabled
    float AdaptiveSpeedThreshold_ms;        // [m/s] Speed threshold for adaptation
    float BrakeDeployAngle_deg;             // [deg] Additional angle when braking
    float ThrottleReductionAngle_deg;       // [deg] Angle reduction at low throttle
    float DamageLevel;                      // [-] Damage level (0.0 = none, 1.0 = destroyed)
    FVector ForceApplicationPoint_COM;      // [m] Force application point relative to COM
};

// --- Underbody / Floor / Diffuser - Physics Thread ---
struct FUnderbody_PT
{
    bool bEnabled = false;                  // [-] Component enabled (socket exists)
    float FloorArea_m2;                     // [m²] Flat floor area
    float DiffuserArea_m2;                  // [m²] Diffuser area
    float DiffuserAngle_deg;                // [deg] Diffuser angle
    float BaseCoeffPressure;                // [-] Base pressure coefficient (negative for suction)
    float DiffuserEfficiency;               // [-] Diffuser efficiency factor (0.0-1.0)
    float RideHeightOptimum_m;              // [m] Optimal ride height
    float RideHeightCritical_m;             // [m] Critical ride height (porpoising threshold)
    float DiffuserDragCoeff;                // [-] Diffuser drag coefficient
    FVector ForceApplicationPoint_COM;      // [m] Force application point relative to COM
};

// --- Side Skirt (Single Side) - Physics Thread ---
struct FSideSkirt_PT
{
    bool bEnabled = false;                  // [-] Component enabled (socket exists)
    float Length_m;                         // [m] Side skirt length
    float Height_m;                         // [m] Side skirt height
    float CoeffPressure;                    // [-] Pressure coefficient (negative for suction)
    float UnderbodySealingEfficiency;       // [-] Sealing efficiency (0.0-1.0)
    float RideHeightSensitivity;            // [-] Ride height sensitivity factor
    float DragCoeff;                        // [-] Drag coefficient
    bool bIsAdaptive;                       // [-] Adaptive side skirt enabled
    float AdaptiveRideHeightThreshold_m;    // [m] Ride height threshold for adaptation
    float DamageLevel;                      // [-] Damage level (0.0 = none, 1.0 = destroyed)
    FVector ForceApplicationPoint_COM;      // [m] Force application point relative to COM
};

// --- Vortex Generators - Physics Thread ---
struct FVortexGenerators_PT
{
    bool bEnabled = false;                  // [-] Component enabled (socket exists)
    float TotalArea_m2;                     // [m²] Total vortex generator area
    float VortexStrength;                   // [-] Vortex strength factor (0.0-1.0)
    float DiffuserEnhancementFactor;        // [-] Diffuser enhancement multiplier (1.0-2.0)
    float DragPenalty;                      // [-] Additional drag coefficient
    float BoundaryLayerControlEfficiency;   // [-] Boundary layer control efficiency (0.0-1.0)
    bool bEnhancesDiffuser;                 // [-] Enhances diffuser performance
    bool bEnhancesUnderbody;                // [-] Enhances underbody performance
    FVector ForceApplicationPoint_COM;      // [m] Force application point relative to COM
};

// --- Body Aerodynamics - Physics Thread ---
struct FBody_PT
{
    bool bEnabled = false;                  // [-] Component enabled (socket exists)
    float FrontalArea_m2;                   // [m²] Vehicle frontal area
    float SideArea_m2;                      // [m²] Vehicle side area
    float CoeffDrag;                        // [-] Body drag coefficient
    float CoeffLift;                        // [-] Body lift coefficient (positive = lift up)
    float CoeffSideForce;                   // [-] Side force coefficient
    FVector ForceApplicationPoint_COM;      // [m] Force application point relative to COM
};

// --- Aerodynamic Package - Physics Thread ---
struct FAerodynamicPackage_PT
{
    float AirDensity_kgm3;                  // [kg/m³] Air density
    FWing_PT RearWing;
    FSplitter_PT FrontSplitter;
    TArray<FCanard_PT> Canards;
    FUnderbody_PT FloorDiffuser;
    FSideSkirt_PT LeftSideSkirt;
    FSideSkirt_PT RightSideSkirt;
    FVortexGenerators_PT VortexGens;
    FBody_PT VehicleBody;
};

// --- Aerodynamic Forces - Physics Thread ---
struct FAerodynamicForces_PT
{
    float TotalDrag_N = 0.0f;               // [N] Total drag force
    float TotalDownforce_N = 0.0f;          // [N] Total downforce (negative lift)
    float FrontDownforce_N = 0.0f;          // [N] Front axle downforce
    float RearDownforce_N = 0.0f;           // [N] Rear axle downforce
    float SideForce_N = 0.0f;               // [N] Side force
    
    FVector DragForceWorld = FVector::ZeroVector;   // [N] Drag force vector in world space
    FVector LiftForceWorld = FVector::ZeroVector;   // [N] Lift force vector in world space
    FVector SideForceWorld = FVector::ZeroVector;   // [N] Side force vector in world space
    
    float PitchMoment_Nm = 0.0f;            // [N⋅m] Pitch moment
    float RollMoment_Nm = 0.0f;             // [N⋅m] Roll moment
    float YawMoment_Nm = 0.0f;              // [N⋅m] Yaw moment
    
    float WingDownforce_N = 0.0f;           // [N] Rear wing downforce
    float WingDrag_N = 0.0f;                // [N] Rear wing drag
    float TotalCanardDownforce_N = 0.0f;    // [N] Total canards downforce
    float TotalCanardDrag_N = 0.0f;         // [N] Total canards drag
    float SplitterDownforce_N = 0.0f;       // [N] Splitter downforce
    float SplitterDrag_N = 0.0f;            // [N] Splitter drag
    float UnderbodyDownforce_N = 0.0f;      // [N] Underbody/diffuser downforce
    float UnderbodyDrag_N = 0.0f;           // [N] Underbody/diffuser drag
    float LeftSideSkirtDownforce_N = 0.0f;  // [N] Left side skirt downforce
    float LeftSideSkirtDrag_N = 0.0f;       // [N] Left side skirt drag
    float RightSideSkirtDownforce_N = 0.0f; // [N] Right side skirt downforce
    float RightSideSkirtDrag_N = 0.0f;      // [N] Right side skirt drag
    float VortexGenDownforceBonus_N = 0.0f; // [N] Vortex generator downforce bonus
    float VortexGenDrag_N = 0.0f;           // [N] Vortex generator drag
    float BodyDrag_N = 0.0f;                // [N] Body drag
    float BodyLift_N = 0.0f;                // [N] Body lift (positive = upward)
};

//------------------------------------------------------------------------------
//                                          GAME THREAD STRUCTURES
//------------------------------------------------------------------------------

// --- Wing / Spoiler (Rear) - Game Thread ---
USTRUCT(BlueprintType)
struct FWing_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float Area_m2 = 1.2f;                   // [m²] Wing surface area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float BaseCoeffLift = -2.5f;            // [-] Base lift coefficient (negative for downforce)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float BaseCoeffDrag = 0.45f;            // [-] Base drag coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float AspectRatio = 3.0f;               // [-] Wing aspect ratio
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float MaxAngle_deg = 18.0f;             // [deg] Maximum wing angle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float MinAngle_deg = 2.0f;              // [deg] Minimum wing angle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float CurrentAngle_deg = 12.0f;         // [deg] Current wing angle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    bool bIsAdaptive = true;                // [-] Adaptive wing enabled
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float AdaptiveSpeedThreshold_ms = 30.0f; // [m/s] Speed threshold
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    float BrakeDeployAngle_deg = 8.0f;      // [deg] Additional angle when braking
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Wing")
    TArray<FName> SocketNames;
};

// --- Splitter / Air Dam (Front) - Game Thread ---
USTRUCT(BlueprintType)
struct FSplitter_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Splitter")
    float Area_m2 = 0.6f;                   // [m²] Splitter surface area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Splitter")
    float CoeffPressure = 0.8f;             // [-] Pressure coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Splitter")
    float AirDamHeight_m = 0.15f;           // [m] Air dam height
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Splitter")
    float AirDamDragCoeff = 0.25f;          // [-] Air dam drag coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Splitter")
    float RideHeightSensitivity = 2.5f;     // [-] Ride height sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Splitter")
    TArray<FName> SocketNames;
};

// --- Canards (Front Wings) - Game Thread ---
USTRUCT(BlueprintType)
struct FCanard_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float Area_m2 = 0.4f;                   // [m²] Canard surface area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float BaseCoeffLift = -1.8f;            // [-] Base lift coefficient (negative for downforce)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float BaseCoeffDrag = 0.35f;            // [-] Base drag coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float AspectRatio = 2.5f;               // [-] Canard aspect ratio
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float MaxAngle_deg = 25.0f;             // [deg] Maximum canard angle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float MinAngle_deg = 0.0f;              // [deg] Minimum canard angle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float CurrentAngle_deg = 8.0f;          // [deg] Current canard angle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    bool bIsAdaptive = true;                // [-] Adaptive canard enabled
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float AdaptiveSpeedThreshold_ms = 25.0f; // [m/s] Speed threshold
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float BrakeDeployAngle_deg = 12.0f;     // [deg] Additional angle when braking
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    float ThrottleReductionAngle_deg = 6.0f; // [deg] Angle reduction at low throttle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DamageLevel = 0.0f;               // [-] Damage level (0.0-1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Canard")
    TArray<FName> SocketNames;
};

// --- Underbody / Floor / Diffuser - Game Thread ---
USTRUCT(BlueprintType)
struct FUnderbody_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float FloorArea_m2 = 2.8f;              // [m²] Flat floor area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float DiffuserArea_m2 = 1.2f;           // [m²] Diffuser area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float DiffuserAngle_deg = 14.0f;        // [deg] Diffuser angle
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float BaseCoeffPressure = -0.65f;       // [-] Base pressure coefficient (negative = suction)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float DiffuserEfficiency = 0.75f;       // [-] Diffuser efficiency (0.0-1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float RideHeightOptimum_m = 0.04f;      // [m] Optimal ride height
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float RideHeightCritical_m = 0.015f;    // [m] Critical ride height
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    float DiffuserDragCoeff = 0.08f;        // [-] Diffuser drag coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Underbody")
    TArray<FName> SocketNames;
};

// --- Side Skirt (Single Side) - Game Thread ---
USTRUCT(BlueprintType)
struct FSideSkirt_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    float Length_m = 1.5f;                  // [m] Side skirt length
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    float Height_m = 0.12f;                 // [m] Side skirt height
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    float CoeffPressure = -0.4f;            // [-] Pressure coefficient (negative = suction)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UnderbodySealingEfficiency = 0.65f; // [-] Sealing efficiency (0.0-1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    float RideHeightSensitivity = 3.0f;     // [-] Ride height sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    float DragCoeff = 0.15f;                // [-] Drag coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    bool bIsAdaptive = true;                // [-] Adaptive side skirt enabled
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    float AdaptiveRideHeightThreshold_m = 0.05f; // [m] Ride height threshold
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DamageLevel = 0.0f;               // [-] Damage level (0.0-1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|SideSkirt")
    TArray<FName> SocketNames;
};

// --- Vortex Generators - Game Thread ---
USTRUCT(BlueprintType)
struct FVortexGenerators_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators")
    float TotalArea_m2 = 0.08f;             // [m²] Total vortex generator area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VortexStrength = 0.7f;            // [-] Vortex strength (0.0-1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators", meta = (ClampMin = "1.0", ClampMax = "2.0"))
    float DiffuserEnhancementFactor = 1.25f; // [-] Diffuser enhancement (1.0-2.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators")
    float DragPenalty = 0.05f;              // [-] Additional drag coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BoundaryLayerControlEfficiency = 0.8f; // [-] Boundary layer control (0.0-1.0)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators")
    bool bEnhancesDiffuser = true;          // [-] Enhances diffuser
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators")
    bool bEnhancesUnderbody = true;         // [-] Enhances underbody
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|VortexGenerators")
    TArray<FName> SocketNames;
};

// --- Body Aerodynamics - Game Thread ---
USTRUCT(BlueprintType)
struct FBody_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Body")
    float FrontalArea_m2 = 2.1f;            // [m²] Vehicle frontal area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Body")
    float SideArea_m2 = 3.5f;               // [m²] Vehicle side area
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Body")
    float CoeffDrag = 0.32f;                // [-] Body drag coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Body")
    float CoeffLift = 0.15f;                // [-] Body lift coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Body")
    float CoeffSideForce = 0.45f;           // [-] Side force coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics|Body")
    TArray<FName> SocketNames;
};

// --- Aerodynamic Package - Game Thread ---
USTRUCT(BlueprintType)
struct FAerodynamicPackage_GT
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    float AirDensity_kgm3 = 1.225f;         // [kg/m³] Air density
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    FWing_GT RearWing;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    FSplitter_GT FrontSplitter;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    TArray<FCanard_GT> Canards;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    FUnderbody_GT FloorDiffuser;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    FSideSkirt_GT LeftSideSkirt;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    FSideSkirt_GT RightSideSkirt;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    FVortexGenerators_GT VortexGens;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
    FBody_GT VehicleBody;
};