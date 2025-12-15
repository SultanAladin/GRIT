#pragma once

#include "CoreMinimal.h"
#include "TireSpecifications.h"
#include "SuspensionSpecifications.h"
#include "BrakingSpecifications.h"
#include "AntiRollbarSpecifications.h"
#include "VehicleAssist.h"
#include "AxleSpecifications.generated.h"

//------------------------------------------------------------------------------
//                          WHEEL POSITION CODES
//------------------------------------------------------------------------------

/** Wheel code definitions for axle positions */
enum class E_WheelCode : int8
{
    None        = 0,
    FrontLeft   = -1,
    RearLeft    = -2,
    MiddleLeft  = -3,
    FrontRight  = 1,
    RearRight   = 2,
    MiddleRight = 3,
    Spare       = 99
};

//------------------------------------------------------------------------------
//                          VEHICLE CLASS PRESETS
//------------------------------------------------------------------------------

/** Vehicle class types for preset configurations */
UENUM(BlueprintType)
enum class E_VehicleClass : uint8
{
    None    UMETA(DisplayName = "None"),
    Default UMETA(DisplayName = "Default"),
    GT3     UMETA(DisplayName = "GT3")
};

//------------------------------------------------------------------------------
//                          AXLE MEMBER (GAME THREAD)
//------------------------------------------------------------------------------

/** Complete specification for a single wheel assembly */
USTRUCT(BlueprintType)
struct FAxleMember
{
    GENERATED_BODY()

    //--------------------------------------------------------------------------
    // CORE IDENTIFIERS
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
    FName AxleID = TEXT("AxleMount_Default");   // [-] - Wheel socket name

    int8 WheelCode = 0;                         // [-] - Wheel position code

    //--------------------------------------------------------------------------
    // COMPONENT SPECIFICATIONS
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    FTireSpecSheet TireSpecifications;          // [-] - Complete tire spec

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    FSuspensionSpecifications SuspensionSpecs;  // [-] - Suspension spec

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    FBrakingSpecifications BrakingSpecifications; // [-] - Braking spec

    //--------------------------------------------------------------------------
    // CONTACT TRACING DATA
    //--------------------------------------------------------------------------
    
    FTrajectoryAtlas TracePattern;              // [-] - Ray casting atlas

    //--------------------------------------------------------------------------
    // CONSTRUCTORS
    //--------------------------------------------------------------------------
    
    FAxleMember() = default;

    /** Construct with wheel position code */
    FAxleMember(E_WheelCode Code) : WheelCode(static_cast<int8>(Code)) {}
};

//------------------------------------------------------------------------------
//                          ASSEMBLY LOADOUT PRESET
//------------------------------------------------------------------------------

/** Populates wheel assembly array and anti-rollbar config for vehicle class */
struct FAssemblyLoadout
{
    /** Build complete 4-wheel GT3 setup (Nissan GT-R Nismo spec) */
    static void LoadGT3Preset(TArray<FAxleMember>& Members, TArray<FAntiRollbar>& Rollbars)
    {
        // Reason: Ensure clean array state
        Members.SetNum(4);
        Rollbars.Empty();

        //----------------------------------------------------------------------
        // FRONT LEFT - Brembo 6-Piston / 410mm
        //----------------------------------------------------------------------
        Members[0].AxleID = TEXT("AxleMount_FL");
        Members[0].WheelCode = static_cast<int8>(E_WheelCode::FrontLeft);
        Members[0].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_FL");
        Members[0].SuspensionSpecs.MaxTravel = 0.30f;                       // [m] - Race spec travel
        Members[0].SuspensionSpecs.Preload = 6000.0f;                       // [N] - Stiff preload
        Members[0].SuspensionSpecs.StaticPreload = 4.0f;                    // [cm] - Race ride height
        Members[0].SuspensionSpecs.MinRaise = 1.5f;                         // [cm] - Limited bump
        Members[0].SuspensionSpecs.MaxDrop = 6.0f;                          // [cm] - Limited droop
        Members[0].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[0].SuspensionSpecs.CurveType = ESuspensionCurveType::Progressive; // [-] - Rising rate
        Members[0].SuspensionSpecs.HardeningFactor = 1.1f;                  // [-] - Softer GT3 front for grip (was 1.8f)
        Members[0].SuspensionSpecs.NaturalFrequency = 2.5f;                 // [Hz] - Stiff setup
        Members[0].SuspensionSpecs.DampingRatio = 0.7f;                     // [-] - High damping
        Members[0].BrakingSpecifications.MaxBrakePressure = 12.0e6f;        // [Pa] - 65% front bias
        Members[0].BrakingSpecifications.DiskOuterRadius = 0.205f;          // [m] - 410mm rotor
        Members[0].BrakingSpecifications.NumPistons = 6;                    // [-] - 6-piston caliper

        //----------------------------------------------------------------------
        // FRONT RIGHT - Brembo 6-Piston / 410mm
        //----------------------------------------------------------------------
        Members[1].AxleID = TEXT("AxleMount_FR");
        Members[1].WheelCode = static_cast<int8>(E_WheelCode::FrontRight);
        Members[1].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_FR");
        Members[1].SuspensionSpecs.MaxTravel = 0.30f;                       // [m] - Race spec travel
        Members[1].SuspensionSpecs.Preload = 6000.0f;                       // [N] - Stiff preload
        Members[1].SuspensionSpecs.StaticPreload = 4.0f;                    // [cm] - Race ride height
        Members[1].SuspensionSpecs.MinRaise = 1.5f;                         // [cm] - Limited bump
        Members[1].SuspensionSpecs.MaxDrop = 6.0f;                          // [cm] - Limited droop
        Members[1].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[1].SuspensionSpecs.CurveType = ESuspensionCurveType::Progressive; // [-] - Rising rate
        Members[1].SuspensionSpecs.HardeningFactor = 1.1f;                  // [-] - Softer GT3 front for grip (was 1.8f)
        Members[1].SuspensionSpecs.NaturalFrequency = 2.5f;                 // [Hz] - Stiff setup
        Members[1].SuspensionSpecs.DampingRatio = 0.7f;                     // [-] - High damping
        Members[1].BrakingSpecifications.MaxBrakePressure = 12.0e6f;        // [Pa] - 65% front bias
        Members[1].BrakingSpecifications.DiskOuterRadius = 0.205f;          // [m] - 410mm rotor
        Members[1].BrakingSpecifications.NumPistons = 6;                    // [-] - 6-piston caliper

        //----------------------------------------------------------------------
        // REAR LEFT - Brembo 4-Piston / 390mm
        //----------------------------------------------------------------------
        Members[2].AxleID = TEXT("AxleMount_RL");
        Members[2].WheelCode = static_cast<int8>(E_WheelCode::RearLeft);
        Members[2].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_RL");
        Members[2].SuspensionSpecs.MaxTravel = 0.30f;                       // [m] - Race spec travel
        Members[2].SuspensionSpecs.Preload = 5500.0f;                       // [N] - Slightly softer rear
        Members[2].SuspensionSpecs.StaticPreload = 4.0f;                    // [cm] - Race ride height
        Members[2].SuspensionSpecs.MinRaise = 1.5f;                         // [cm] - Limited bump
        Members[2].SuspensionSpecs.MaxDrop = 6.0f;                          // [cm] - Limited droop
        Members[2].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[2].SuspensionSpecs.CurveType = ESuspensionCurveType::Progressive; // [-] - Rising rate
        Members[2].SuspensionSpecs.HardeningFactor = 1.6f;                  // [-] - Moderate hardening
        Members[2].SuspensionSpecs.NaturalFrequency = 2.3f;                 // [Hz] - Slightly softer rear
        Members[2].SuspensionSpecs.DampingRatio = 0.65f;                    // [-] - Moderate damping
        Members[2].BrakingSpecifications.MaxBrakePressure = 8.0e6f;         // [Pa] - 35% rear bias
        Members[2].BrakingSpecifications.DiskOuterRadius = 0.195f;          // [m] - 390mm rotor
        Members[2].BrakingSpecifications.NumPistons = 4;                    // [-] - 4-piston caliper

        //----------------------------------------------------------------------
        // REAR RIGHT - Brembo 4-Piston / 390mm
        //----------------------------------------------------------------------
        Members[3].AxleID = TEXT("AxleMount_RR");
        Members[3].WheelCode = static_cast<int8>(E_WheelCode::RearRight);
        Members[3].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_RR");
        Members[3].SuspensionSpecs.MaxTravel = 0.30f;                       // [m] - Race spec travel
        Members[3].SuspensionSpecs.Preload = 5500.0f;                       // [N] - Slightly softer rear
        Members[3].SuspensionSpecs.StaticPreload = 4.0f;                    // [cm] - Race ride height
        Members[3].SuspensionSpecs.MinRaise = 1.5f;                         // [cm] - Limited bump
        Members[3].SuspensionSpecs.MaxDrop = 6.0f;                          // [cm] - Limited droop
        Members[3].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[3].SuspensionSpecs.CurveType = ESuspensionCurveType::Progressive; // [-] - Rising rate
        Members[3].SuspensionSpecs.HardeningFactor = 1.6f;                  // [-] - Moderate hardening
        Members[3].SuspensionSpecs.NaturalFrequency = 2.3f;                 // [Hz] - Slightly softer rear
        Members[3].SuspensionSpecs.DampingRatio = 0.65f;                    // [-] - Moderate damping
        Members[3].BrakingSpecifications.MaxBrakePressure = 8.0e6f;         // [Pa] - 35% rear bias
        Members[3].BrakingSpecifications.DiskOuterRadius = 0.195f;          // [m] - 390mm rotor
        Members[3].BrakingSpecifications.NumPistons = 4;                    // [-] - 4-piston caliper

        //----------------------------------------------------------------------
        // ANTI-ROLLBARS - Front (0-1) + Rear (2-3)
        //----------------------------------------------------------------------
        FAntiRollbar FrontBar;
        FrontBar.bEnabled = true;
        FrontBar.Stiffness = 60000.0f;                                      // [N/m] - Softer front bar to reduce understeer (was 85000)
        FrontBar.Damping = 1800.0f;                                         // [N·s/m] - Front damping
        FrontBar.WheelIndices[0] = 0;                                       // [-] - FL index
        FrontBar.WheelIndices[1] = 1;                                       // [-] - FR index

        FAntiRollbar RearBar;
        RearBar.bEnabled = true;
        RearBar.Stiffness = 95000.0f;                                       // [N/m] - Rear roll stiffness
        RearBar.Damping = 2000.0f;                                          // [N·s/m] - Rear damping
        RearBar.WheelIndices[0] = 2;                                        // [-] - RL index
        RearBar.WheelIndices[1] = 3;                                        // [-] - RR index

        Rollbars.Add(FrontBar);
        Rollbars.Add(RearBar);
    } // End LoadGT3Preset

    /** Build default 4-wheel setup */
    static void LoadDefaultPreset(TArray<FAxleMember>& Members, TArray<FAntiRollbar>& Rollbars)
    {
        // Reason: Ensure clean array state
        Members.SetNum(4);
        Rollbars.Empty();

        //----------------------------------------------------------------------
        // FRONT LEFT
        //----------------------------------------------------------------------
        Members[0].AxleID = TEXT("AxleMount_FL");
        Members[0].WheelCode = static_cast<int8>(E_WheelCode::FrontLeft);
        Members[0].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_FL");
        Members[0].SuspensionSpecs.MaxTravel = 0.35f;                       // [m] - Standard travel
        Members[0].SuspensionSpecs.Preload = 5000.0f;                       // [N] - Default preload
        Members[0].SuspensionSpecs.StaticPreload = 5.0f;                    // [cm] - Standard ride height
        Members[0].SuspensionSpecs.MinRaise = 2.0f;                         // [cm] - Bump limit
        Members[0].SuspensionSpecs.MaxDrop = 8.0f;                          // [cm] - Droop limit
        Members[0].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[0].SuspensionSpecs.CurveType = ESuspensionCurveType::Digressive; // [-] - Digressive curve
        Members[0].SuspensionSpecs.HardeningFactor = 1.5f;                  // [-] - Moderate hardening
        Members[0].SuspensionSpecs.NaturalFrequency = 2.0f;                 // [Hz] - Comfort setup
        Members[0].SuspensionSpecs.DampingRatio = 0.6f;                     // [-] - Standard damping

        //----------------------------------------------------------------------
        // FRONT RIGHT
        //----------------------------------------------------------------------
        Members[1].AxleID = TEXT("AxleMount_FR");
        Members[1].WheelCode = static_cast<int8>(E_WheelCode::FrontRight);
        Members[1].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_FR");
        Members[1].SuspensionSpecs.MaxTravel = 0.35f;                       // [m] - Standard travel
        Members[1].SuspensionSpecs.Preload = 5000.0f;                       // [N] - Default preload
        Members[1].SuspensionSpecs.StaticPreload = 5.0f;                    // [cm] - Standard ride height
        Members[1].SuspensionSpecs.MinRaise = 2.0f;                         // [cm] - Bump limit
        Members[1].SuspensionSpecs.MaxDrop = 8.0f;                          // [cm] - Droop limit
        Members[1].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[1].SuspensionSpecs.CurveType = ESuspensionCurveType::Digressive; // [-] - Digressive curve
        Members[1].SuspensionSpecs.HardeningFactor = 1.5f;                  // [-] - Moderate hardening
        Members[1].SuspensionSpecs.NaturalFrequency = 2.0f;                 // [Hz] - Comfort setup
        Members[1].SuspensionSpecs.DampingRatio = 0.6f;                     // [-] - Standard damping

        //----------------------------------------------------------------------
        // REAR LEFT
        //----------------------------------------------------------------------
        Members[2].AxleID = TEXT("AxleMount_RL");
        Members[2].WheelCode = static_cast<int8>(E_WheelCode::RearLeft);
        Members[2].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_RL");
        Members[2].SuspensionSpecs.MaxTravel = 0.35f;                       // [m] - Standard travel
        Members[2].SuspensionSpecs.Preload = 5000.0f;                       // [N] - Default preload
        Members[2].SuspensionSpecs.StaticPreload = 5.0f;                    // [cm] - Standard ride height
        Members[2].SuspensionSpecs.MinRaise = 2.0f;                         // [cm] - Bump limit
        Members[2].SuspensionSpecs.MaxDrop = 8.0f;                          // [cm] - Droop limit
        Members[2].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[2].SuspensionSpecs.CurveType = ESuspensionCurveType::Digressive; // [-] - Digressive curve
        Members[2].SuspensionSpecs.HardeningFactor = 1.5f;                  // [-] - Moderate hardening
        Members[2].SuspensionSpecs.NaturalFrequency = 2.0f;                 // [Hz] - Comfort setup
        Members[2].SuspensionSpecs.DampingRatio = 0.6f;                     // [-] - Standard damping

        //----------------------------------------------------------------------
        // REAR RIGHT
        //----------------------------------------------------------------------
        Members[3].AxleID = TEXT("AxleMount_RR");
        Members[3].WheelCode = static_cast<int8>(E_WheelCode::RearRight);
        Members[3].SuspensionSpecs.SuspensionID = TEXT("SuspensionMount_RR");
        Members[3].SuspensionSpecs.MaxTravel = 0.35f;                       // [m] - Standard travel
        Members[3].SuspensionSpecs.Preload = 5000.0f;                       // [N] - Default preload
        Members[3].SuspensionSpecs.StaticPreload = 5.0f;                    // [cm] - Standard ride height
        Members[3].SuspensionSpecs.MinRaise = 2.0f;                         // [cm] - Bump limit
        Members[3].SuspensionSpecs.MaxDrop = 8.0f;                          // [cm] - Droop limit
        Members[3].SuspensionSpecs.bUseProgressive = true;                  // [-] - Progressive enabled
        Members[3].SuspensionSpecs.CurveType = ESuspensionCurveType::Digressive; // [-] - Digressive curve
        Members[3].SuspensionSpecs.HardeningFactor = 1.5f;                  // [-] - Moderate hardening
        Members[3].SuspensionSpecs.NaturalFrequency = 2.0f;                 // [Hz] - Comfort setup
        Members[3].SuspensionSpecs.DampingRatio = 0.6f;                     // [-] - Standard damping

        //----------------------------------------------------------------------
        // ANTI-ROLLBARS - Front (0-1) + Rear (2-3)
        //----------------------------------------------------------------------
        FAntiRollbar FrontBar;
        FrontBar.bEnabled = true;
        FrontBar.Stiffness = 80000.0f;                                      // [N/m] - Front roll stiffness
        FrontBar.Damping = 1500.0f;                                         // [N·s/m] - Front damping
        FrontBar.WheelIndices[0] = 0;                                       // [-] - FL index
        FrontBar.WheelIndices[1] = 1;                                       // [-] - FR index

        FAntiRollbar RearBar;
        RearBar.bEnabled = true;
        RearBar.Stiffness = 80000.0f;                                       // [N/m] - Rear roll stiffness
        RearBar.Damping = 1500.0f;                                          // [N·s/m] - Rear damping
        RearBar.WheelIndices[0] = 2;                                        // [-] - RL index
        RearBar.WheelIndices[1] = 3;                                        // [-] - RR index

        Rollbars.Add(FrontBar);
        Rollbars.Add(RearBar);
    } // End LoadDefaultPreset
};


/* 

Doesent seem i need it 

//------------------------------------------------------------------------------
//                          AXLE ANCHOR METADATA
//------------------------------------------------------------------------------

/ Complete wheel mounting metadata /
USTRUCT(BlueprintType)
struct FAxleAnchor
{
    GENERATED_BODY()

    //--------------------------------------------------------------------------
    // POSITION DATA
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
    int8 Side = -1;                                 // [-] - Left=-1, Right=1, Center=0

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
    int8 AxleRow = 0;                               // [-] - Front=0, Middle=1, Rear=2, Spare=99

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
    FName SocketID = TEXT("AxleMount_Default");     // [-] - Socket name

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
    bool bMounted = true;                           // [-] - Tire installed at anchor

    //--------------------------------------------------------------------------
    // DRIVETRAIN DATA
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drivetrain")
    bool PowertrainCoupled = false;                 // [-] - Mechanically linked to engine

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drivetrain")
    bool SteeringEnabled = false;                   // [-] - Steer angle input enabled

    //--------------------------------------------------------------------------
    // CONSTRUCTORS
    //--------------------------------------------------------------------------
    
    FAxleAnchor() = default;

    / Construct from position data /
    FAxleAnchor(int8 InSide, int8 InAxleRow, FName InSocket, bool bPowertrainCoupled = false, bool bSteeringEnabled = false) : Side(InSide), AxleRow(InAxleRow), SocketID(InSocket), PowertrainCoupled(bPowertrainCoupled), SteeringEnabled(bSteeringEnabled) {}

    //--------------------------------------------------------------------------
    // UTILITY FUNCTIONS
    //--------------------------------------------------------------------------
    
    / Generate compact identifier string (e.g. "FL", "RR", "MidL") /
    FString CompactID() const
    {
        FString AxleStr;
        if (AxleRow == 0) AxleStr = TEXT("F");
        else if (AxleRow == 1) AxleStr = TEXT("Mid");
        else if (AxleRow == 2) AxleStr = TEXT("R");
        else if (AxleRow == 99) AxleStr = TEXT("Spare");

        FString SideStr;
        if (Side == -1) SideStr = TEXT("L");
        else if (Side == 1) SideStr = TEXT("R");
        else if (Side == 0) SideStr = TEXT("C");

        return AxleStr + SideStr;
    } // End CompactID

    / Check if wheel is on front axle 
    bool IsFront() const { return AxleRow == 0; }

    / Check if wheel is on rear axle 
    bool IsRear() const { return AxleRow == 2; }

    / Check if wheel is on left side 
    bool IsLeft() const { return Side == -1; }

    / Check if wheel is on right side 
    bool IsRight() const { return Side == 1; }
};

//------------------------------------------------------------------------------
//                          WHEELSET ARRAY PRESETS
//------------------------------------------------------------------------------

/ Initialize standard 4-wheel configuration 
inline TArray<FAxleAnchor> BuildWheelset_Quad()
{
    TArray<FAxleAnchor> Wheelset;
    Wheelset.SetNum(4);

    // Reason: Initialize front-left anchor
    Wheelset[0] = FAxleAnchor(-1, 0, TEXT("AxleMount_FL"), false, true);
    Wheelset[0].bMounted = true;

    // Reason: Initialize front-right anchor
    Wheelset[1] = FAxleAnchor(1, 0, TEXT("AxleMount_FR"), false, true);
    Wheelset[1].bMounted = true;

    // Reason: Initialize rear-left anchor
    Wheelset[2] = FAxleAnchor(-1, 2, TEXT("AxleMount_RL"), true, false);
    Wheelset[2].bMounted = true;

    // Reason: Initialize rear-right anchor
    Wheelset[3] = FAxleAnchor(1, 2, TEXT("AxleMount_RR"), true, false);
    Wheelset[3].bMounted = true;

    return Wheelset;
} // End BuildWheelset_Quad

*/