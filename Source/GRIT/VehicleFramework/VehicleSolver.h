

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "Chaos/SimCallbackObject.h"
#include "PBDRigidsSolver.h"
#include "Chaos/ParticleHandle.h"
#include "Components/TireSpecifications.h"
#include "Components/SuspensionSpecifications.h"
#include "Components/AntiRollbarSpecifications.h"
#include "Components/AxleSpecifications.h"
#include "Components/BrakingSpecifications.h"
#include "Components/SteeringAssembly.h"
#include "Components/ChassisConfiguration.h"
#include "Components/DrivetrainSpecifications.h"
#include "Components/AerodynamicSpecifications.h"
#include "Input/InputTensor.h"
#include "Telemetry/TelemetryLogger.h"
#include "VehicleSolver.generated.h"

//------------------------------------------------------------------------------------------------------------------------
//                          FORWARD DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

class UStaticMeshComponent;
class ATireConstruct;
class AVehicleSolver;


//------------------------------------------------------------------------------------------------------------------------
//                                              VEHICLE STATE DATA
//------------------------------------------------------------------------------------------------------------------------
/* Instantaneous vehicle state snapshot - captures complete rigid body state and derived quantities for physics simulation */
struct FInstantaneousVehicleRecord
{
    //--------------------------------------------------------------------------------------------------------------------
    //                                               Mass & geometry
    //--------------------------------------------------------------------------------------------------------------------
    float μ_mass;                                          // [kg] - Total vehicle mass
    FTransform HullXfm;                                    // [-] - Chassis world transform
    FVector σ_centerOfMass;                                // [cm] - Center of mass world position
    float L_wheelbase;                                     // [cm] - Front-rear axle distance
    float W_trackwidth;                                    // [cm] - Left-right wheel distance
    float W_halftrack;                                     // [cm] - Half track width
    bool β_geometry_initialized;                           // [-] - Geometry setup complete

    //--------------------------------------------------------------------------------------------------------------------
    //                                     Linear velocity (multiple units)
    //--------------------------------------------------------------------------------------------------------------------
    FVector ν_linearCms;                                   // [cm/s] - Linear velocity (Unreal units)
    FVector ν_linearMs;                                    // [m/s] - Linear velocity (SI units)
    float ν_forwardMs;                                     // [m/s] - Longitudinal velocity
    float ν_lateralMs;                                     // [m/s] - Lateral velocity
    float ν_verticalMs;                                    // [m/s] - Vertical velocity
    float ν_magnitudeMs;                                   // [m/s] - Speed magnitude
    float ν_magnitudeKmh;                                  // [km/h] - Speed in km/h
    float ν_forwardCms;                                    // [cm/s] - Longitudinal velocity (cm)
    float ν_lateralCms;                                    // [cm/s] - Lateral velocity (cm)
    float ν_magnitudeCms;                                  // [cm/s] - Speed magnitude (cm)

    //--------------------------------------------------------------------------------------------------------------------
    //                                               Angular velocity
    //--------------------------------------------------------------------------------------------------------------------
    FVector ω_angularRads;                                 // [rad/s] - Angular velocity vector
    float ψ_yaw_rate;                                      // [rad/s] - Yaw rotation rate
    float θ_pitch_rate;                                    // [rad/s] - Pitch rotation rate
    float φ_roll_rate;                                     // [rad/s] - Roll rotation rate

    //--------------------------------------------------------------------------------------------------------------------
    //                                              Orientation basis vectors
    //--------------------------------------------------------------------------------------------------------------------
    FVector ê_longitudinal;                                // [-] - Forward unit vector
    FVector ê_lateral;                                     // [-] - Right unit vector
    FVector ê_vertical;                                    // [-] - Up unit vector

    //--------------------------------------------------------------------------------------------------------------------
    //                                                  Linear acceleration
    //--------------------------------------------------------------------------------------------------------------------
    FVector α_linear;                                      // [m/s²] - Linear acceleration vector
    float α_longitudinal;                                  // [m/s²] - Forward acceleration
    float α_lateral;                                       // [m/s²] - Lateral acceleration
    float α_g_longitudinal;                                // [g] - Longitudinal g-force
    float α_g_lateral;                                     // [g] - Lateral g-force

    //--------------------------------------------------------------------------------------------------------------------
    //  Angular acceleration
    //--------------------------------------------------------------------------------------------------------------------
    FVector α_angular;                                     // [rad/s²] - Angular acceleration vector

    //--------------------------------------------------------------------------------------------------------------------
    //  External forces
    //--------------------------------------------------------------------------------------------------------------------
    float F_gravity_slope;                                 // [N] - Gravitational force component
    float F_net_longitudinal;                              // [N] - Net forward force
    float F_net_lateral;                                   // [N] - Net lateral force

    //--------------------------------------------------------------------------------------------------------------------
    //  State flags
    //--------------------------------------------------------------------------------------------------------------------
    bool β_stationary_total;                               // [-] - Vehicle fully stopped
    bool β_stationary_longitudinal;                        // [-] - No forward motion
    bool β_stationary_lateral;                             // [-] - No lateral motion
    bool β_stationary_rotational;                          // [-] - No rotation
    bool β_airborne;                                       // [-] - No ground contact

    //--------------------------------------------------------------------------------------------------------------------
    //  Stability metrics
    //--------------------------------------------------------------------------------------------------------------------
    float ζ_stability_factor;                              // [-] - Stability coefficient

    //--------------------------------------------------------------------------------------------------------------------
    //  Persistent history for finite-difference acceleration estimates
    //--------------------------------------------------------------------------------------------------------------------
    FVector PreviousVelocity;                              // [cm/s] - Last frame linear velocity
    FVector PreviousAngularVelocity;                       // [rad/s] - Last frame angular velocity

    FInstantaneousVehicleRecord()
        : μ_mass(1720.0f), HullXfm(FTransform::Identity), σ_centerOfMass(FVector::ZeroVector),
          L_wheelbase(265.0f), W_trackwidth(150.0f), W_halftrack(75.0f),
          β_geometry_initialized(false), 
          ν_linearCms(FVector::ZeroVector), ν_linearMs(FVector::ZeroVector),
          ν_forwardMs(0.0f), ν_lateralMs(0.0f), ν_verticalMs(0.0f), 
          ν_magnitudeMs(0.0f), ν_magnitudeKmh(0.0f), 
          ν_forwardCms(0.0f), ν_lateralCms(0.0f), ν_magnitudeCms(0.0f),
          ω_angularRads(FVector::ZeroVector), 
          ψ_yaw_rate(0.0f), θ_pitch_rate(0.0f), φ_roll_rate(0.0f),
          ê_longitudinal(FVector::ForwardVector), ê_lateral(FVector::RightVector), ê_vertical(FVector::UpVector),
          α_linear(FVector::ZeroVector), α_longitudinal(0.0f), α_lateral(0.0f), 
          α_g_longitudinal(0.0f), α_g_lateral(0.0f), α_angular(FVector::ZeroVector),
          F_gravity_slope(0.0f), F_net_longitudinal(0.0f), F_net_lateral(0.0f), 
          β_stationary_total(true), β_stationary_longitudinal(true), 
          β_stationary_lateral(true), β_stationary_rotational(true), β_airborne(false),
          ζ_stability_factor(1.0f),
          PreviousVelocity(FVector::ZeroVector),
          PreviousAngularVelocity(FVector::ZeroVector)
    {}
}; // End FInstantaneousVehicleRecord


/*====================================================================================================================================
                                                         DATA CONDUITS & SYNC
======================================================================================================================================*/

//------------------------------------------------------------------------------
//                                CONFIG SYNC BITFLAGS
//------------------------------------------------------------------------------

namespace FInterlockedBitmask
{
    using Type = uint32;
    enum : Type
    {
        None         = 0,
        Suspension   = 1 << 0, // [-] - Bit 0
        AxleLayout   = 1 << 1, // [-] - Bit 1
        TireGeom     = 1 << 2, // [-] - Bit 2
        Braking      = 1 << 3, // [-] - Bit 3
        Drivetrain   = 1 << 4, // [-] - Bit 4
        AntiRollbar  = 1 << 5  // [-] - Bit 5
    };
}

//------------------------------------------------------------------------------
//                           COMM STATUS REGISTER (BITMASK)
//------------------------------------------------------------------------------

/* Atomic bitmask for GT <-> PT communication - prevents false sharing and enables lock-free synchronization */
struct FSyncEventFlags
{
private:
    TAtomic<FInterlockedBitmask::Type> Flags{FInterlockedBitmask::None}; // [-] - Atomic configuration flags

public:
    /** [GT] Set flags to signal new data is available */
    void Signal(FInterlockedBitmask::Type InFlags)
    {
        FInterlockedBitmask::Type CurrentFlags = Flags.Load();
        while (!Flags.CompareExchange(CurrentFlags, CurrentFlags | InFlags)) // Reason: atomic CAS loop for thread safety
        {
        } // End while (CAS loop)
    } // End Signal()

    /** [PT] Check if specific flags are set */
    bool IsSignaled(FInterlockedBitmask::Type InFlags) const { return (Flags.Load() & InFlags) != 0; }
    
    /** [PT] Acknowledge and clear flags after processing data */
    void Acknowledge(FInterlockedBitmask::Type InFlags)
    {
        FInterlockedBitmask::Type CurrentFlags = Flags.Load();
        while (!Flags.CompareExchange(CurrentFlags, CurrentFlags & ~InFlags)) // Reason: atomic CAS loop for thread safety
        {
        } // End while (CAS loop)
    } // End Acknowledge()
}; // End FSyncEventFlags

//------------------------------------------------------------------------------
//                              GENERIC DATA CONDUIT (GT -> PT)
//------------------------------------------------------------------------------

/* Thread-safe, single-producer, single-consumer, double-buffered data channel - enables lock-free GT->PT data transfer */
template<typename T>
class TThreadLock
{
private:
    T               Buffers[2];                // [-] - Double buffer for data
    TAtomic<int32>  WriteIndex{0};             // [-] - Index for GT to write, PT to read

public:
    /** [GT] Publish new data to the back buffer and swap */
    void Publish(const T& Data)
    {
        const int32 BackBufferIdx = 1 - WriteIndex.Load();
        Buffers[BackBufferIdx] = Data;
        WriteIndex.Store(BackBufferIdx);
    } // End Publish()

    /** [PT] Get a const reference to the latest published data */
    const T& Read() const { return Buffers[WriteIndex.Load()]; }
}; // End TThreadLock
//------------------------------------------------------------------------------
//                          INVARIANT WHEEL METRICS CACHE
//------------------------------------------------------------------------------
/* Precomputed wheel geometry constants - eliminates redundant calculations during contact tracing */
struct FInvariantWheelMetrics
{
    float RadiusCm;                                        // [cm] - Wheel outer radius
    float WidthCm;                                         // [cm] - Tire contact patch width
    float HalfWidthCm;                                     // [cm] - Half tire width for lateral offset
    float RestLengthCm;                                    // [cm] - Suspension rest length
    float MaxDropCm;                                       // [cm] - Maximum suspension extension
    float MaxRaiseCm;                                      // [cm] - Maximum suspension compression
    
    /** Cached trace endpoint offsets */
    FVector UpRaiseOffset;                                 // [cm] - UpDir * MaxRaise precomputed
    FVector DownDropOffset;                                // [cm] - DownDir * (RestLength + MaxDrop) precomputed
    
    FInvariantWheelMetrics()
        : RadiusCm(0.f), WidthCm(0.f), HalfWidthCm(0.f)
        , RestLengthCm(0.f), MaxDropCm(0.f), MaxRaiseCm(0.f)
        , UpRaiseOffset(FVector::ZeroVector), DownDropOffset(FVector::ZeroVector)
    {}
}; // End FInvariantWheelMetrics

//------------------------------------------------------------------------------
//                    SUSPENSION RUNTIME STATE (SoA - Physics Thread)
//------------------------------------------------------------------------------
/* Physics thread axle data - Structure-of-Arrays layout optimized for cache-friendly sequential access during simulation */
struct FVehicleSolverAxleData_PT
{
    //---------------------------------------------------------------------------------------------------------
    //                             IDENTIFIERS (Never changes)
    //---------------------------------------------------------------------------------------------------------
    TArray<FName> AxleIDs;                           // [-] - Axle socket names
    TArray<int8> WheelCodes;                         // [-] - Wheel position codes (FL/FR/RL/RR)

    //---------------------------------------------------------------------------------------------------------
    //                             SPECIFICATIONS (Setup only - read-only at runtime)
    //---------------------------------------------------------------------------------------------------------
    TArray<FSuspensionSpecifications> SuspensionSpecs; // [-] - Spring/damper configuration
    TArray<FTireSpecification> TireSpecifications;     // [-] - Tire parameters
    TArray<FBrakingSpecifications> BrakingSpecs;       // [-] - Brake configuration
    TArray<FTrajectoryAtlas> TracePatterns;            // [-] - Multi-ray trace patterns
    TArray<FInvariantWheelMetrics> WheelMetricsCache;  // [-] - Precomputed geometry

    //---------------------------------------------------------------------------------------------------------
    //                   🔥 SUSPENSION STATE - HOT DATA (accessed every frame)
    //---------------------------------------------------------------------------------------------------------
    TArray<float> SpringDisplacements;               // [cm] - Current compression/extension
    TArray<float> SpringVelocities;                  // [cm/s] - Rate of change of displacement
    TArray<float> SprungMasses;                      // [kg] - Per-wheel sprung masses
    TArray<float> SpringForces;                      // [N] - Current spring force output
    TArray<float> WheelLoads;                        // [N] - Dynamic normal load (includes load transfer)
    TArray<float> StaticLoads_LT;                    // [N] - Baseline loads for advanced load transfer (per wheel)
    TArray<float> ComputedLoads_LT;                  // [N] - Computed wheel loads from advanced load transfer
    TArray<bool>  bIsInContact;                      // [-] - Wheel touching ground
    TArray<FVector> WorldSpringOrigins;              // [cm] - World-space mount point
    
    //---------------------------------------------------------------------------------------------------------
    //                   SUSPENSION AXIS - Per-wheel suspension travel direction (world-space, computed each frame)
    //---------------------------------------------------------------------------------------------------------
    TArray<FVector> LocalSuspensionAxes;             // [-] - Local-space suspension axis (mirrored for right wheels)
    TArray<FVector> WorldSuspensionAxes;             // [-] - World-space suspension axis (transformed each frame)

    //---------------------------------------------------------------------------------------------------------
    //                   TIRE STATE - HOT DATA (accessed every frame)
    //---------------------------------------------------------------------------------------------------------
    TArray<float> SlipAnglesRad;                     // [rad] - Lateral slip angle
    TArray<float> LongitudinalSlips;                 // [-] - Longitudinal slip ratio (κ)
    TArray<float> SteerAnglesRad;                    // [rad] - Steering input angle
    TArray<float> AngularVelocities;                 // [rad/s] - Wheel spin rate
    TArray<float> RotationAngles;                    // [rad] - Accumulated rotation (for visuals)
    TArray<float> LongitudinalForces;                // [N] - Fx tire force
    TArray<float> LateralForces;                     // [N] - Fy tire force
    TArray<bool>  WheelLocked;                       // [-] - Static friction lock state

    //---------------------------------------------------------------------------------------------------------
    //                   DRIVETRAIN STATE - HOT DATA
    //---------------------------------------------------------------------------------------------------------
    TArray<float> DriveTorquesNm;                    // [N·m] - Drive torque from powertrain
    TArray<float> BrakeTorquesNm;                    // [N·m] - Brake torque applied

    //---------------------------------------------------------------------------------------------------------
    //                   CONTACT STATE - HOT DATA
    //---------------------------------------------------------------------------------------------------------
    TArray<FVector> AverageContactLocations;         // [cm] - Ground contact point
    TArray<FVector> AverageContactNormals;           // [-] - Ground normal vector
    TArray<int32> ActiveContactCounts;               // [-] - Number of trace hits

    //---------------------------------------------------------------------------------------------------------
    //                   TRANSIENT TIRE STATE - PERSISTENT (survives across frames)
    //---------------------------------------------------------------------------------------------------------
    TArray<float> Fx_Transient;                      // [N] - Longitudinal force with transient lag
    TArray<float> Fy_Transient;                      // [N] - Lateral force with transient lag
    TArray<float> SlipEnergyRate_W;                  // [W] - Instantaneous slip power dissipation

    //---------------------------------------------------------------------------------------------------------
    //                   DERIVED/SECONDARY STATE (Less frequently accessed)
    //---------------------------------------------------------------------------------------------------------
    TArray<float> RollingResistanceForceN;           // [N] - Calculated resistance force
    TArray<float> RollingResistanceTorqueNm;         // [N·m] - Calculated resistance torque
    TArray<float> PneumaticTrail_m;                  // [m] - Self-aligning moment arm
    TArray<float> SelfAligningTorques;               // [N·m] - Mz tire moment
    TArray<float> GradeForceN;                       // [N] - Gravity component on slope

    //---------------------------------------------------------------------------------------------------------
    //                   TIRE PHYSICS - PERSISTENT (Thermal/Wear - slow changing)
    //---------------------------------------------------------------------------------------------------------
    TArray<float> TireSurfaceTemp_K;                 // [K] - Tread temperature
    TArray<float> TireCoreTemp_K;                    // [K] - Carcass core temperature
    TArray<float> TirePressure_Pa;                   // [Pa] - Inflation pressure
    TArray<float> TireWearDepth_mm;                  // [mm] - Remaining tread depth
    TArray<float> TireDistance_km;                   // [km] - Distance traveled
    TArray<float> AccumulatedSlipEnergy_MJ;          // [MJ] - Cumulative slip work
    TArray<float> HydroplaningRatio;                 // [0-1] - Hydroplaning severity
    TArray<float> WaterFilmThickness_mm;             // [mm] - Water depth at patch

    //--------------------------------------------------------------------------
    //                          METHODS
    //--------------------------------------------------------------------------
    
    /** Resize all arrays to the specified count */
    void SetNum(int32 Count)
    {
        // Identifiers
        AxleIDs.SetNum(Count);
        WheelCodes.SetNum(Count);
        
        // Specifications
        SuspensionSpecs.SetNum(Count);
        TireSpecifications.SetNum(Count);
        BrakingSpecs.SetNum(Count);
        TracePatterns.SetNum(Count);
        WheelMetricsCache.SetNum(Count);
        
        // Suspension state
        SpringDisplacements.SetNum(Count);
        SpringVelocities.SetNum(Count);
        SprungMasses.SetNum(Count);
        SpringForces.SetNum(Count);
        WheelLoads.SetNum(Count);
        StaticLoads_LT.SetNum(Count);
        ComputedLoads_LT.SetNum(Count);
        bIsInContact.SetNum(Count);
        WorldSpringOrigins.SetNum(Count);
        LocalSuspensionAxes.SetNum(Count);
        WorldSuspensionAxes.SetNum(Count);
        
        // Tire state
        SlipAnglesRad.SetNum(Count);
        LongitudinalSlips.SetNum(Count);
        SteerAnglesRad.SetNum(Count);
        AngularVelocities.SetNum(Count);
        RotationAngles.SetNum(Count);
        LongitudinalForces.SetNum(Count);
        LateralForces.SetNum(Count);
        WheelLocked.SetNum(Count);

        // Drivetrain
        DriveTorquesNm.SetNum(Count);
        BrakeTorquesNm.SetNum(Count);
        
        // Contact
        AverageContactLocations.SetNum(Count);
        AverageContactNormals.SetNum(Count);
        ActiveContactCounts.SetNum(Count);
        
        // Transient tire state
        Fx_Transient.SetNum(Count);
        Fy_Transient.SetNum(Count);
        SlipEnergyRate_W.SetNum(Count);

        // Secondary
        RollingResistanceForceN.SetNum(Count);
        RollingResistanceTorqueNm.SetNum(Count);
        PneumaticTrail_m.SetNum(Count);
        SelfAligningTorques.SetNum(Count);
        GradeForceN.SetNum(Count);
        
        // Tire physics
        TireSurfaceTemp_K.SetNum(Count);
        TireCoreTemp_K.SetNum(Count);
        TirePressure_Pa.SetNum(Count);
        TireWearDepth_mm.SetNum(Count);
        TireDistance_km.SetNum(Count);
        AccumulatedSlipEnergy_MJ.SetNum(Count);
        HydroplaningRatio.SetNum(Count);
        WaterFilmThickness_mm.SetNum(Count);
    } // End SetNum()

    /** Zero per-frame dynamic state */
    void ZeroDynamicState()
    {
        const int32 Num = AxleIDs.Num();
        for (int32 i = 0; i < Num; ++i)
        {
            // Reset per-frame suspension state
            SpringDisplacements[i] = 0.0f;
            SpringVelocities[i] = 0.0f;
            SprungMasses[i] = 0.0f;
            SpringForces[i] = 0.0f;
            WheelLoads[i] = 0.0f;
            StaticLoads_LT[i] = 0.0f;
            ComputedLoads_LT[i] = 0.0f;
            bIsInContact[i] = false;
            WorldSpringOrigins[i] = FVector::ZeroVector;
            
            // Reset per-frame tire state
            LongitudinalSlips[i] = 0.0f;
            SteerAnglesRad[i] = 0.0f;
            WheelLocked[i] = false;
            
            // Reset per-frame drivetrain
            BrakeTorquesNm[i] = 0.0f;
            
            // Reset per-frame contact
            AverageContactLocations[i] = FVector::ZeroVector;
            AverageContactNormals[i] = FVector::UpVector;
            ActiveContactCounts[i] = 0;
            
            // Reset secondary state
            RollingResistanceForceN[i] = 0.0f;
            RollingResistanceTorqueNm[i] = 0.0f;
            PneumaticTrail_m[i] = 0.0f;
            SelfAligningTorques[i] = 0.0f;
            GradeForceN[i] = 0.0f;
        } // End for (state reset)
        
        // Note: Persistent state (AngularVelocities, RotationAngles, SlipAnglesRad,
        // LongitudinalForces, LateralForces, DriveTorquesNm, TireTemp, TireWear, etc.)
        // is intentionally NOT reset - these values carry over between frames
    } // End ZeroDynamicState()

/** Initialize from FAxleMember array */
    void InitializeFromAxleMembers(const TArray<FAxleMember>& AxleMembers)
    {
        const int32 Count = AxleMembers.Num();
        SetNum(Count);
        
        for (int32 i = 0; i < Count; ++i)
        {
            const FAxleMember& Member = AxleMembers[i];
            
            // Copy identifiers
            AxleIDs[i] = Member.AxleID;
            WheelCodes[i] = Member.WheelCode;
            
            // Copy specifications (use assignment operator for conversion)
            SuspensionSpecs[i] = Member.SuspensionSpecs;
            TireSpecifications[i] = Member.TireSpecifications; // Converts FTireSpecSheet -> FTireSpecification
            BrakingSpecs[i] = Member.BrakingSpecifications;
            TracePatterns[i] = Member.TracePattern;
            
            // Initialize persistent state
            WheelLocked[i] = false;
            AngularVelocities[i] = 0.0f;
            RotationAngles[i] = 0.0f;
            
            // Initialize suspension axis with mirroring for right-side wheels
            // WheelCode: FrontLeft=1, FrontRight=2, RearLeft=3, RearRight=4
            // Right wheels (FR=2, RR=4) mirror the Y component of the axis
            const bool bIsRightWheel = (Member.WheelCode == 2 || Member.WheelCode == 4);
            FVector LocalAxis = Member.SuspensionSpecs.LocalSuspensionAxis.GetSafeNormal();
            if (bIsRightWheel)
            {
                LocalAxis.Y = -LocalAxis.Y;  // Mirror Y for right-side wheels
            }
            LocalSuspensionAxes[i] = LocalAxis;
            WorldSuspensionAxes[i] = LocalAxis;  // Will be transformed each frame
            
            // Initialize tire physics to ambient conditions
            TireSurfaceTemp_K[i] = 293.15f; // 20°C
            TireCoreTemp_K[i] = 293.15f;
            TirePressure_Pa[i] = Member.TireSpecifications.NominalPressure_Pa; // Use flat structure
            TireWearDepth_mm[i] = 8.0f; // New tire
            TireDistance_km[i] = 0.0f;
            AccumulatedSlipEnergy_MJ[i] = 0.0f;
            HydroplaningRatio[i] = 0.0f;
            WaterFilmThickness_mm[i] = 0.0f;
        } // End for (axle initialization)
        
        ZeroDynamicState();
    } // End InitializeFromAxleMembers()

   /** Rebuild wheel metrics cache from specifications (legacy - uses global up/down) */
    void RebuildWheelMetricsCache(const FVector& UpDir, const FVector& DownDir)
    {
        if (WheelMetricsCache.Num() != AxleIDs.Num())
            WheelMetricsCache.SetNum(AxleIDs.Num());

        for (int32 i = 0; i < AxleIDs.Num(); ++i)
        {
            FInvariantWheelMetrics& Cache = WheelMetricsCache[i];
            const FTireSpecification& Tire = TireSpecifications[i];
            const FSuspensionSpecifications& Susp = SuspensionSpecs[i];

            Cache.RadiusCm = Tire.Mechanical.Geometry.OuterRadius * 100.f; // [m] -> [cm]
            Cache.WidthCm = Tire.Mechanical.Geometry.Width * 100.f;        // [m] -> [cm]
            Cache.HalfWidthCm = Cache.WidthCm * 0.5f;
            Cache.RestLengthCm = Susp.RestLength;
            Cache.MaxDropCm = Susp.MaxDrop;
            Cache.MaxRaiseCm = Susp.MinRaise;
            Cache.UpRaiseOffset = UpDir * Cache.MaxRaiseCm;
            Cache.DownDropOffset = DownDir * (Cache.RestLengthCm + Cache.MaxDropCm);
        } // End for (wheel metrics)
    } // End RebuildWheelMetricsCache()
    
    /** 
     * Rebuild wheel metrics cache using per-wheel suspension axes.
     * Call this after transforming LocalSuspensionAxes to WorldSuspensionAxes.
     */
    void RebuildWheelMetricsCachePerAxis()
    {
        if (WheelMetricsCache.Num() != AxleIDs.Num())
            WheelMetricsCache.SetNum(AxleIDs.Num());

        for (int32 i = 0; i < AxleIDs.Num(); ++i)
        {
            FInvariantWheelMetrics& Cache = WheelMetricsCache[i];
            const FTireSpecification& Tire = TireSpecifications[i];
            const FSuspensionSpecifications& Susp = SuspensionSpecs[i];
            
            // Use per-wheel suspension axis (LocalSuspensionAxis points DOWN = compression direction)
            const FVector& SuspAxis = WorldSuspensionAxes[i];  // Already transformed to world space
            const FVector UpAxis = -SuspAxis;                   // Opposite of compression = extension

            Cache.RadiusCm = Tire.Mechanical.Geometry.OuterRadius * 100.f; // [m] -> [cm]
            Cache.WidthCm = Tire.Mechanical.Geometry.Width * 100.f;        // [m] -> [cm]
            Cache.HalfWidthCm = Cache.WidthCm * 0.5f;
            Cache.RestLengthCm = Susp.RestLength;
            Cache.MaxDropCm = Susp.MaxDrop;
            Cache.MaxRaiseCm = Susp.MinRaise;
            Cache.UpRaiseOffset = UpAxis * Cache.MaxRaiseCm;               // Extension direction
            Cache.DownDropOffset = SuspAxis * (Cache.RestLengthCm + Cache.MaxDropCm); // Compression direction
        } // End for (wheel metrics)
    } // End RebuildWheelMetricsCachePerAxis()
}; // End FVehicleSolverAxleData_PT

//------------------------------------------------------------------------------
//                          ENVIRONMENT DATA
//------------------------------------------------------------------------------
/** Environment state - ambient conditions for simulation */
struct FEnvironmentData
{
    float AmbientTemperature_K = 293.15f;            // [K] - Current outside air temperature
    float AmbientPressure_Pa = 101325.0f;            // [Pa] - Atmospheric pressure
    float OxygenConcentration = 0.2095f;             // [-] - O2 fraction
    float AirFilterEfficiency = 1.0f;                // [-] - Air filter flow coefficient
};

//------------------------------------------------------------------------------
//                          AERODYNAMIC FORCES (Precomputed)
//------------------------------------------------------------------------------
/**
 * Precomputed aerodynamic forces - computed ONCE per frame and passed to subsystems
 * 
 * USAGE:
 * - Downforce: Applied to wheel loads in ComputeLoadTransferRealtime() (increases tire Fz → more grip)
 * - Drag: Applied to chassis in SolvePowertrain() (decelerates vehicle)
 * - Pitch Moment: Optional chassis torque for front/rear aero imbalance
 * 
 * IMPORTANT: Downforce is NOT applied directly to chassis - only through wheel loads!
 * This prevents double-counting (downforce increases grip, not vehicle weight).
 */
struct FAerodynamicForces
{
    float TotalDrag_N = 0.0f;                        // [N] - Horizontal drag force (opposes motion)
    float TotalDownforce_N = 0.0f;                   // [N] - Total vertical downforce (positive = down)
    
    float FrontDownforce_N = 0.0f;                   // [N] - Front axle downforce
    float RearDownforce_N = 0.0f;                    // [N] - Rear axle downforce
    
    FVector DragForceWorld = FVector::ZeroVector;   // [N] - Drag vector in world space (opposes velocity)
    FVector DownforceTorque_Nm = FVector::ZeroVector; // [N·m] - Pitch moment about CoM (optional)
};

//------------------------------------------------------------------------------
//                          PHYSICS CALLBACK INPUT/OUTPUT
//------------------------------------------------------------------------------
/* Physics callback input data - passed from game thread to physics thread each frame */
struct FVehicleSolverInput : public Chaos::FSimCallbackInput
{
#if !UE_BUILD_SHIPPING
    int32 FrameNumber = 0;                               // [-] - Debug frame counter
#endif
    UWorld* World = nullptr;                             // [-]
    FCollisionQueryParams TraceParams;                   // [-]
    FCollisionObjectQueryParams ObjectQueryParams;       // [-] - Precomputed object query params (WorldStatic + WorldDynamic)
    FCollisionResponseContainer TraceResponse;           // [-]

    void Reset()
    {
#if !UE_BUILD_SHIPPING
        FrameNumber = 0;
#endif
        World = nullptr;
        TraceParams = FCollisionQueryParams();
        ObjectQueryParams = FCollisionObjectQueryParams();
        TraceResponse = FCollisionResponseContainer::GetDefaultResponseContainer();
    } // End Reset()
}; // End FVehicleSolverInput

/* Physics callback output data - returned from physics thread to game thread */
struct FVehicleSolverOutput : public Chaos::FSimCallbackOutput
{
    void Reset() {}
}; // End FVehicleSolverOutput

//------------------------------------------------------------------------------
//                          P2P REPLICATED STATE
//------------------------------------------------------------------------------
// Vehicle state for P2P replication - uses full precision FVector/FRotator
// for UHT compatibility (USTRUCT required for UPROPERTY)

USTRUCT(BlueprintType)
struct FReplicatedVehicleState
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Position = FVector::ZeroVector;      // [cm] - World position

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;   // [deg] - World rotation

    UPROPERTY()
    FVector LinearVelocity = FVector::ZeroVector; // [cm/s] - Linear velocity

    UPROPERTY()
    FVector AngularVelocity = FVector::ZeroVector; // [rad/s] - Angular velocity

    UPROPERTY()
    float EngineRPM = 0.0f;                       // [rpm] - Engine speed

    UPROPERTY()
    int32 Gear = 0;                               // [-] - Current gear

    UPROPERTY()
    float Throttle = 0.0f;                        // [0-1] - Throttle position

    UPROPERTY()
    float Brake = 0.0f;                           // [0-1] - Brake position

    UPROPERTY()
    uint16 FrameNumber = 0;                       // [-] - Physics frame counter

    /** Interpolate between two states */
    static FReplicatedVehicleState Lerp(const FReplicatedVehicleState& A,
                                        const FReplicatedVehicleState& B,
                                        float Alpha)
    {
        FReplicatedVehicleState Result;
        Result.Position = FMath::Lerp(A.Position, B.Position, Alpha);
        Result.Rotation = FMath::Lerp(A.Rotation, B.Rotation, Alpha);
        Result.LinearVelocity = B.LinearVelocity;
        Result.AngularVelocity = B.AngularVelocity;
        Result.EngineRPM = FMath::Lerp(A.EngineRPM, B.EngineRPM, Alpha);
        Result.Gear = B.Gear;
        Result.Throttle = B.Throttle;
        Result.Brake = B.Brake;
        Result.FrameNumber = B.FrameNumber;
        return Result;
    } // End Lerp()
}; // End FReplicatedVehicleState

//------------------------------------------------------------------------------
//                          P2P REPLICATED INPUT
//------------------------------------------------------------------------------
// Client input sent to host for processing

USTRUCT(BlueprintType)
struct FReplicatedVehicleInput
{
    GENERATED_BODY()

    UPROPERTY()
    float Throttle = 0.0f;         // [0-1] - Throttle input

    UPROPERTY()
    float Brake = 0.0f;            // [0-1] - Brake input

    UPROPERTY()
    float Steering = 0.0f;         // [-1,1] - Steering input

    UPROPERTY()
    float Clutch = 0.0f;           // [0-1] - Clutch input

    UPROPERTY()
    int8 GearRequest = 0;          // [-1,14] - Requested gear (-1=R, 0=N, 1+=forward)

    UPROPERTY()
    uint8 InputFlags = 0;          // [-] - Bit flags (handbrake, boost, etc.)

    UPROPERTY()
    uint32 InputSequence = 0;      // [-] - Sequence number for ordering

    // Flag bit positions
    static constexpr uint8 FLAG_HANDBRAKE = 0x01;
    static constexpr uint8 FLAG_BOOST = 0x02;

    void SetHandbrake(bool bActive) { InputFlags = bActive ? (InputFlags | FLAG_HANDBRAKE) : (InputFlags & ~FLAG_HANDBRAKE); }
    void SetBoost(bool bActive) { InputFlags = bActive ? (InputFlags | FLAG_BOOST) : (InputFlags & ~FLAG_BOOST); }
    bool GetHandbrake() const { return (InputFlags & FLAG_HANDBRAKE) != 0; }
    bool GetBoost() const { return (InputFlags & FLAG_BOOST) != 0; }
}; // End FReplicatedVehicleInput

//------------------------------------------------------------------------------
//                          P2P EVENT TYPES
//------------------------------------------------------------------------------
// Event types for Multicast_VehicleEvent RPC

enum class EP2PVehicleEvent : uint8
{
    None = 0,
    GearChange = 1,        // EventData = new gear
    Collision = 2,         // EventData = collision intensity
    EngineStart = 3,       // EventData = unused
    EngineStop = 4,        // EventData = unused
    Boost = 5,             // EventData = boost level
    Backfire = 6,          // EventData = unused
}; // End EP2PVehicleEvent

//------------------------------------------------------------------------------
//                          FRICTION STATE MACHINE TELEMETRY
//------------------------------------------------------------------------------
#if !UE_BUILD_SHIPPING

/** Sample struct for friction state machine telemetry */
struct FFrictionStateSample
{
    float Time_s = 0.0f;                    // [s] - Simulation time
    float DeltaTime_s = 0.0f;               // [s] - Physics timestep
    int32 WheelIndex = 0;                   // [-] - Wheel index (0-3)
    int32 StateEnum = 0;                    // [-] - 0=Kinetic, 1=Static_Lateral, 2=Static_Full
    float VehicleSpeed_ms = 0.0f;           // [m/s] - Vehicle speed magnitude
    float Vx_ms = 0.0f;                     // [m/s] - Longitudinal velocity (wheel frame)
    float Vy_ms = 0.0f;                     // [m/s] - Lateral velocity (wheel frame)
    float ContactSpeed_ms = 0.0f;           // [m/s] - Contact patch speed
    float WheelOmega_rads = 0.0f;           // [rad/s] - Wheel angular velocity
    float BrakeTorque_Nm = 0.0f;            // [N·m] - Brake torque applied
    float ClutchTorque_Nm = 0.0f;           // [N·m] - Clutch torque
    float DriveTorque_Nm = 0.0f;            // [N·m] - Drive torque to wheel
    float F_Slope_Long_N = 0.0f;            // [N] - Longitudinal slope force (gravity component)
    float F_Slope_Lat_N = 0.0f;             // [N] - Lateral slope force (gravity component)
    float F_TireFrictionLimit_N = 0.0f;     // [N] - Max tire friction capacity
    float F_MechHoldLimit_N = 0.0f;         // [N] - Mechanical hold capacity (brakes + engine)
    float Fx_Applied_N = 0.0f;              // [N] - Final longitudinal force applied
    float Fy_Applied_N = 0.0f;              // [N] - Final lateral force applied
    float WheelLoad_N = 0.0f;               // [N] - Vertical wheel load
    bool bTireCanHoldLong = false;          // [-] - Tire has grip for longitudinal hold
    bool bMechCanHoldLong = false;          // [-] - Brakes can hold longitudinal
}; // End FFrictionStateSample

#endif // !UE_BUILD_SHIPPING

//------------------------------------------------------------------------------
//                          PRECOMPUTED STEERING GEOMETRY (PHYSICS THREAD)
//------------------------------------------------------------------------------
/* Cached steering geometry values - computed once during initialization to avoid redundant calculations every frame */
struct FSteeringGeometryCache_PT
{
    float WheelBaseCm = 0.0f;        // [cm] - Distance between front and rear axles (converted from meters)
    float HalfTrackCm = 0.0f;        // [cm] - Half the distance between left and right wheels (converted from meters)
    bool bInitialized = false;       // [-] - Whether cache has been computed

    /** Initialize cache from chassis configuration */
    void Initialize(const FChassisConfiguration& ChassisConfig)
    {
        WheelBaseCm = ChassisConfig.WheelBase * 100.0f;       // [m] -> [cm]
        HalfTrackCm = 0.5f * ChassisConfig.TrackWidth * 100.0f; // [m] -> [cm]
        bInitialized = true;
    } // End Initialize()

    /** Reset cache to uninitialized state */
    void Reset()
    {
        WheelBaseCm = 0.0f;
        HalfTrackCm = 0.0f;
        bInitialized = false;
    } // End Reset()
}; // End FSteeringGeometryCache_PT

//------------------------------------------------------------------------------
//                          physics callback (simulation thread)
//------------------------------------------------------------------------------
/* Physics simulation callback - executes suspension and tire simulation on physics thread before each Chaos solver step */
class FVehicleSolverCallback : public Chaos::TSimCallbackObject<FVehicleSolverInput, FVehicleSolverOutput>
{
    //------------------------------------------------------------------------------
    //                          public api
    //------------------------------------------------------------------------------
public:
    FVehicleSolverCallback();

    //------------------------------------------------------------------------------
    //                          ownership and runtime links
    //------------------------------------------------------------------------------
    AVehicleSolver* VehicleOwner = nullptr;              // [-]
    Chaos::FSingleParticlePhysicsProxy* Proxy = nullptr; // [-]

    /** Physics thread axle data */
    FVehicleSolverAxleData_PT AxleData_PT;               // [-]

    /** PT-local suspension configuration */
    FVehicleSuspensionConfig SuspensionConfig_PT;        // [-]

    /** Physics thread anti-rollbar configuration */
    TArray<FAntiRollbar> AntiRollbars_PT;                // [-]

    /** Last computed anti-roll torque for telemetry */
    FVector LastAntiRollTorque = FVector::ZeroVector;    // [N·cm]

    /** Physics thread steering assembly */
    FSteeringAssembly SteeringSystem_PT;                 // [-]

    /** Physics thread chassis configuration */
    FChassisConfiguration ChassisConfig_PT;              // [-]

    /** Precomputed steering geometry cache (avoids redundant calculations) */
    FSteeringGeometryCache_PT SteeringGeometryCache_PT;  // [-]

    /** Load transfer runtime state */
    struct FLoadTransferState
    {
        TArray<float> StaticLoads_N;          // [N]
        TArray<float> DeltaLong_N;            // [N]
        TArray<float> DeltaLat_N;             // [N]
        TArray<float> AeroSplit_N;            // [N]
        float CGHeight_m = 0.55f;             // [m]
        float Wheelbase_m = 2.6f;             // [m]
        float TrackFront_m = 1.6f;            // [m]
        float TrackRear_m = 1.6f;             // [m]
        float Lf_m = 1.3f;                    // [m]
        float Lr_m = 1.3f;                    // [m]
        void SetNum(int32 Count)
        { StaticLoads_N.SetNum(Count); DeltaLong_N.SetNum(Count); DeltaLat_N.SetNum(Count); AeroSplit_N.SetNum(Count); }
    } LoadXfer_PT;

    /** Pointer to game thread input conduit (set during initialization) */
    TThreadLock<FInputTensor>* InputConduitPtr = nullptr; // [-] - Link to GT input

    /** Physics thread drivetrain specifications (immutable) */
    FDrivetrainSpecifications DrivetrainSpecs_PT;        // [-]

    /** Physics thread aerodynamics package */
    FAerodynamicPackage_PT AeroPackage_PT;               // [-] - Aerodynamics configuration (PT)

    /** Physics thread aerodynamic forces (computed each frame) */
    FAerodynamicForces_PT AeroForces_PT;                 // [-] - Current frame aero forces (PT)

    /** Physics thread drivetrain state (mutable) */
    FDrivetrainStateVector DrivetrainState_PT;           // [-]

    /** Physics thread brake state vectors (thermal model) */
    TArray<FBrakingStateVector> BrakeStates_PT;          // [-] - Per-wheel brake thermal state

    /** Previous wheel angular velocities for filtering/smoothing */
    TArray<float> PrevWheelOmegas_PT;                    // [rad·s⁻¹] - Previous wheel speeds

    /** Environment data (ambient conditions) */
    FEnvironmentData Environment_PT;                     // [-] - Ambient temperature, pressure, etc.


#if !UE_BUILD_SHIPPING
    //------------------------------------------------------------------------------
    //                          TELEMETRY (Dev/Editor only)
    //------------------------------------------------------------------------------
    /** Current frame telemetry sample */
    FTelemetrySample CurrentTelemetrySample;             // [-] - Current frame telemetry data

    /** Current frame performance sample */
    FPerformanceSample CurrentPerfSample;                // [-] - Current frame performance data

    /** Static wheel loads for load transfer calculation */
    TArray<float> StaticWheelLoads;                      // [N] - Baseline static loads

    /** Collect telemetry sample for current frame */
    FTelemetrySample CollectTelemetrySample(float SimTime, float DeltaTime, const FInstantaneousVehicleRecord& Rec, const FInputTensor& Input);

    /** Collect performance sample for current frame */
    FPerformanceSample CollectPerformanceSample(float SimTime, float DeltaTime);

    /** Current frame aerodynamics sample */
    FAerodynamicsSample CurrentAeroSample;               // [-] - Current frame aero data

    /** Collect aerodynamics sample for current frame */
    FAerodynamicsSample CollectAerodynamicsSample(float SimTime, const FInstantaneousVehicleRecord& Rec, const FAerodynamicForces_PT& AeroForces, float MinRideHeight_cm);
#endif // !UE_BUILD_SHIPPING

    //------------------------------------------------------------------------------
    //                          overrides
    //------------------------------------------------------------------------------
private:
    virtual void OnPreSimulate_Internal() override;
    
    //------------------------------------------------------------------------------
    //                          simulation pipeline (sim thread)
    //------------------------------------------------------------------------------
    void ComputeSuspensionDisplacements(const FVehicleSolverInput* Input, int32 SpringCount, const FTransform& HullXfm, const FVector& UpDir, const FVector& DownDir, FVehicleSolverAxleData_PT& AxleData);
    void ComputeSuspensionForces(const FInstantaneousVehicleRecord& Rec, const FVector& LinearVel, const FVector& AngularVelRad, int32 SpringCount, FVehicleSolverAxleData_PT& AxleData);
    void ComputeLoadTransferLagrange(const FInstantaneousVehicleRecord& ChassisRec, float DeltaTime, int32 NumWheels, FVehicleSolverAxleData_PT& AxleData);
    void ComputeLoadTransferRealtime(const FInstantaneousVehicleRecord& ChassisRec, float DeltaTime, int32 NumWheels, FVehicleSolverAxleData_PT& AxleData, const FAerodynamicForces_PT& AeroForces);
    
    /* Compute anti-roll bar forces and apply them to the rigid body */
    void ComputeAntiRollbarForces(Chaos::FRigidBodyHandle_Internal* RigidBody, const FVehicleSolverAxleData_PT& AxleData);

    /* Process steering input and compute wheel steer angles */
    void ProcessSteering(float DeltaTime, float SteeringInput, const FTransform& HullXfm, FVehicleSolverAxleData_PT& AxleData);

    /** Hybrid kinematic/Newton slip solver with transient tire response and energy tracking */
    void SolveContactSlip(int32 WheelIndex, float DeltaTime, const FInstantaneousVehicleRecord& Rec, FVehicleSolverAxleData_PT& AxleData);

    //------------------------------------------------------------------------------
    //                          POWERTRAIN SIMULATION
    //------------------------------------------------------------------------------

    /** Solve complete powertrain - engine, clutch, transmission, differential, wheel dynamics */
    void SolvePowertrain(float DeltaTime, const FInstantaneousVehicleRecord& Rec, const FInputTensor& Input, Chaos::FRigidBodyHandle_Internal* RigidBody, FVehicleSolverAxleData_PT& AxleData, const FAerodynamicForces_PT& AeroForces);

    //------------------------------------------------------------------------------
    //                          AERODYNAMICS SIMULATION
    //------------------------------------------------------------------------------

    /**
     * Compute all aerodynamic forces ONCE per frame
     * 
     * This function computes body drag, body downforce, underbody ground effect,
     * and distributes downforce to front/rear axles based on center of pressure.
     * 
     * USAGE PATTERN:
     * 1. Call ONCE in OnPreSimulate_Internal() BEFORE load transfer
     * 2. Pass result to ComputeLoadTransferRealtime() for wheel load distribution
     * 3. Pass result to SolvePowertrain() for drag application to chassis
     * 
     * @param Aero Aerodynamic package configuration (wings, splitter, diffuser, etc.)
     * @param VehicleSpeed_ms Vehicle speed in m/s
     * @param RideHeight_m Minimum ride height in meters (for ground effect)
     * @param VelocityDir_World Normalized velocity direction in world space
     * @param BrakeInput Brake pedal position [0-1] for adaptive wing deployment
     * @param ThrottleInput Throttle pedal position [0-1] for drag reduction systems
     * @param YawRate_rads Yaw rate in rad/s for side force calculation (optional, default 0)
     * @return FAerodynamicForces_PT struct with all computed forces
     */
    FAerodynamicForces_PT ComputeAerodynamicForces(
        const FAerodynamicPackage_PT& Aero,
        float VehicleSpeed_ms,
        float RideHeight_m,
        const FVector& VelocityDir_World,
        float BrakeInput,
        float ThrottleInput,
        float YawRate_rads = 0.0f
    );


    /** Compute combined Pacejka forces using MF6.1 model - returns FVector(Fx, Fy, Mz) */
    FVector ComputeCombinedPacejkaForces(int32 WheelIndex, float Kappa, float Alpha, float Camber, const FVehicleSolverAxleData_PT& AxleData) const;

    /** Helper function for SolveTireSlipNewton - computes combined forces with out parameters */
    void ComputeCombinedPacejkaForces(int32 WheelIndex, float SlipRatio, float SlipAngleRad, float CamberRad, const FVehicleSolverAxleData_PT& AxleData, float& OutFx, float& OutFy) const;

    //------------------------------------------------------------------------------
    //                          PACEJKA MF6.1 TIRE MODEL
    //------------------------------------------------------------------------------

    /** Compute longitudinal force using Pacejka MF6.1 with horizontal/vertical shifts */
    float ComputePacejkaLongitudinalForce(int32 WheelIndex, float SlipRatio, float CamberRad, const FVehicleSolverAxleData_PT& AxleData) const;

    /** Compute lateral force using Pacejka MF6.1 with horizontal/vertical shifts and camber thrust */
    float ComputePacejkaLateralForce(int32 WheelIndex, float SlipAngleRad, float CamberRad, const FVehicleSolverAxleData_PT& AxleData) const;

    /** Compute self-aligning torque with combined slip correction (fixes brake-while-steering) */
    float ComputeSelfAligningTorque(int32 WheelIndex, float SlipAngleRad, float LongitudinalSlip, float LateralForceN, float CamberRad, const FVehicleSolverAxleData_PT& AxleData) const;

    /** Apply combined slip using MF6.1 weighting functions (replaces friction ellipse) */
    FVector2D ApplyCombinedSlip(int32 WheelIndex, float LongitudinalForce, float LateralForce, float SlipRatio, float SlipAngleRad, float CamberRad, const FVehicleSolverAxleData_PT& AxleData) const;

    /** Compute cornering stiffness at current operating point (dFy/dα at α=0) */
    float ComputeCorneringStiffness(int32 WheelIndex, const FVehicleSolverAxleData_PT& AxleData) const;

    /** Determine if tire is operating in linear region */
    bool IsInLinearRegion(int32 WheelIndex, float SlipAngleRad, float LateralForceN, const FVehicleSolverAxleData_PT& AxleData) const;


    //------------------------------------------------------------------------------
    //                          math utilities
    //------------------------------------------------------------------------------

    /** Solve linear system via Gaussian elimination on physics thread */
    TArray<float> SolveLinearSystemGaussian(const TArray<TArray<float>>& A, const TArray<float>& b);
    
}; // End FVehicleSolverCallback


//------------------------------------------------------------------------------
//                          vehicle solver pawn
//------------------------------------------------------------------------------
/* High-fidelity vehicle dynamics solver - manages suspension, tires, and physics simulation on both game and physics threads */
UCLASS()
class GRIT_API AVehicleSolver : public APawn
{
    GENERATED_BODY()

    //------------------------------------------------------------------------------
    //                          public api
    //------------------------------------------------------------------------------
public:
    /** Constructor */
    AVehicleSolver();

    /** Core lifecycle */
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Component lifecycle */
    virtual void PostRegisterAllComponents() override;
    virtual void PostInitializeComponents() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    /** Networking */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
    /** Editor support */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    /** Physics initialization */
    void InitializePhysics();
    void ShutdownPhysics();

    /** Physics callback instance */
    FVehicleSolverCallback* PhysicsCallback = nullptr;              // [-]
    Chaos::FSingleParticlePhysicsProxy* CachedPhysicsProxy = nullptr; // [-]
    bool bPhysicsCallbackInitialized = false;                       // [-]
    
    /** Retrieve current tire configuration */
    void RetrieveTireConfiguration();
    
    /** Apply Pacejka lateral ply-steer symmetry using WheelCode (left/right mirroring) */
    void ApplyPacejkaPlySteerSymmetry();
    
    /** Configure anti-roll bar system */
    void ConfigureAntiRollbars();
    
    /** Retrieve current anti-roll bar configuration */
    void RetrieveAntiRollbarConfiguration();

        /** Initialize aerodynamics package with socket locations */
    void InitializeAerodynamicsPackage();

    /** Configure aerodynamics package for physics thread */
    void ConfigureAerodynamicsPackage();

public:
    //--------------------------------------------------------------------------
    //                          CORE COMPONENTS
    //--------------------------------------------------------------------------
    
    /** Vehicle hull mesh component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    UStaticMeshComponent* VehicleHull;

    //--------------------------------------------------------------------------
    //                          GAME THREAD DATA (SINGLE SOURCE OF TRUTH)
    //--------------------------------------------------------------------------
    
    /** 
     * Array of axle members - contains ALL wheel specifications and configuration
     *  AoS (Array-of-Structs) is optimal for GT:
     * - Random access patterns (editor tweaks)
     * - Infrequent modifications
     * - UPROPERTY reflection support
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Axles")
    TArray<FAxleMember> Axles_GT;                        // [-] - Complete axle configuration

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Axles")
    TArray<FAntiRollbar> AntiRollbars_GT;                    // [-] - Anti-rollbar configuration

    /** Game thread steering assembly */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
    FSteeringAssembly SteeringSystem_GT;                 // [-] - Steering configuration

    /** Game thread chassis configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Chassis")
    FChassisConfiguration ChassisConfig_GT;              // [-] - Chassis geometry

    /** Game thread input tensor (player/AI input state) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Input")
    FInputTensor InputTensor_GameThread;                 // [-] - Player input state

    /** Game thread to physics thread input conduit (thread-safe double buffer) */
    TThreadLock<FInputTensor> InputConduit;              // [-] - GT -> PT input channel

    /** Game thread drivetrain specifications (engine, clutch, transmission, differential) */
    FDrivetrainSpecifications Drivetrain_GT;             // [-] - Drivetrain configuration (GT)

       /** Game thread aerodynamics package */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Aerodynamics")
    FAerodynamicPackage_GT AeroPackage_GT;  

    //--------------------------------------------------------------------------
    //                          AERODYNAMICS CONFIGURATION (GT)
    //--------------------------------------------------------------------------
  
    
    /** Helper: Get world locations of sockets from a component
     *  @param Component - Component containing the sockets
     *  @param SocketNames - Array of socket names to query
     *  @return TArray<FVector> - World space positions of all valid sockets */
    TArray<FVector> GetSocketWorldLocations(USceneComponent* Component, const TArray<FName>& SocketNames) const;

    /** Tire construct registry (source actors that provide specs/meshes, not ticked) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tires")
    TArray<ATireConstruct*> TireRegistry;                // [-] - Tire source registry (hidden when mounted)

    /** Instanced static mesh components for visual representation (auto-managed) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Tires")
    TArray<UStaticMeshComponent*> TireMeshInstances;     // [-] - Runtime visual proxies

    //--------------------------------------------------------------------------
    //                          SUSPENSION TRACING CONFIGURATION
    //--------------------------------------------------------------------------

    /** 
     * Use simplified single-ray trace per wheel (FAST MODE)
     * - TRUE: 4 raycasts total (1 per wheel) - ~40-60μs
     * - DepthCount=1: Single centered trace per direction (symmetric for all wheels)
     * - DepthCount>1: Multi-ray fan pattern across tire width
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension|Tracing", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MaxAngle = 45.0f;                               // [deg] - Max fan angle for multi-ray traces

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension|Tracing", meta = (ClampMin = "0.1"))
    float AngleStep = 5.0f;                               // [deg] - Angular spacing between rays

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension|Tracing", meta = (ClampMin = "1"))
    int32 DepthCount = 1;                                 // [-] - Number of lateral samples (1 = centered, >1 = spread across tire width)

    
        /** Suspension configuration shared with physics thread */
    FVehicleSuspensionConfig SuspensionConfig;           // [-]

    /** Physics thread vehicle state cache (updated each frame) */
    FInstantaneousVehicleRecord VehicleRecord_PhysicsThread; // [-]

    //--------------------------------------------------------------------------
    //                          TELEMETRY SYSTEM (Dev/Editor Only)
    //--------------------------------------------------------------------------
    
    /** Enable powertrain CSV telemetry logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Telemetry")
    bool bEnableTelemetryLogging = true;                 // [-] - Enable/disable CSV logging

    /** Enable performance profiling CSV logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Telemetry")
    bool bEnablePerformanceLogging = true;              // [-] - Enable/disable performance CSV

    /** Enable aerodynamics CSV logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Telemetry")
    bool bEnableAerodynamicsLogging = true;             // [-] - Enable/disable aero CSV

    /** Enable aerodynamics debug visualization (ride height, force vectors) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Debug")
    bool bEnableAeroDebugDraw = false;                  // [-] - Enable/disable aero debug drawing

    /** Cached minimum ride height from physics thread (for GT debug visualization) */
    float CachedRideHeight_m = 0.0f;                    // [m] - Latest ride height from PT

#if !UE_BUILD_SHIPPING
    /** Telemetry sample buffer - populated during gameplay, written to CSV in EndPlay */
    TArray<FTelemetrySample> TelemetrySampleBuffer;      // [-] - Buffered telemetry samples

    /** Performance sample buffer - populated during gameplay, written to CSV in EndPlay */
    TArray<FPerformanceSample> PerformanceSampleBuffer;  // [-] - Buffered performance samples

    /** Aerodynamics sample buffer - populated during gameplay, written to CSV in EndPlay */
    TArray<FAerodynamicsSample> AerodynamicsSampleBuffer; // [-] - Buffered aero samples

    /** Friction state sample buffer - populated during physics, written to CSV in EndPlay */
    TArray<FFrictionStateSample> FrictionStateSampleBuffer; // [-] - Buffered friction state samples

    /** Enable friction state logging */
    bool bEnableFrictionStateLogging = true;             // [-] - Enable/disable friction CSV

    /** Friction log sample interval (log every N physics steps) */
    int32 FrictionLogInterval = 5;                       // [-] - Sample every N frames

    /** Friction log frame counter */
    int32 FrictionLogFrameCounter = 0;                   // [-] - Frame counter for interval

    /** Static wheel loads for load transfer calculation */
    TArray<float> StaticWheelLoads_GT;                   // [N] - Baseline static loads

    /** Write telemetry buffer to CSV file */
    void WriteTelemetryCsv();

    /** Write performance buffer to CSV file */
    void WritePerformanceCsv();

    /** Write aerodynamics buffer to CSV file */
    void WriteAerodynamicsCsv();

    /** Write friction state buffer to CSV file */
    void WriteFrictionStateCsv();

    /** Draw debug visualization for aerodynamics (ride height, downforce, drag) */
    void DrawAerodynamicsDebug();
#endif // !UE_BUILD_SHIPPING

    //--------------------------------------------------------------------------
    //                          TIRE MOUNT SYSTEM
    //--------------------------------------------------------------------------
    
    /** Register tire specifications and create visual proxy */
    bool MountTire(ATireConstruct* TireSource, int32 WheelIndex);

    /** Unregister tire and destroy visual proxy */
    ATireConstruct* DismountTire(int32 WheelIndex);

    /** Batch tire registration from registry */
    void RegisterTires();
    
    /** Calculate mass distribution across springs using Lagrange multipliers */
    bool ComputeSuspensionSprungMasses(const TArray<FVector>& LocalSpringPositions, const FVector& LocalCenterOfMass, const float TotalMass, TArray<float>& OutSprungMasses);

protected:
    /** Initialize axle data for the requested vehicle class */
    void InitializeAxles(E_VehicleClass VehicleClass);

    /** Generate ray-fan atlas for suspension tracing */
    void GenerateTrajectoryAtlas();

    /**
     * Initialize axle topology based on vehicle class
     * Sets up wheel positions, tire specs, and contact tracing patterns
     */
    void InitializeAxleTopology_GameThread(E_VehicleClass VehicleClass);

    /**
     * Create visual proxy for mounted tire
     * Clones mesh from TireConstruct and attaches to vehicle
     */
    UStaticMeshComponent* CreateTireVisualProxy(ATireConstruct* TireSource, int32 WheelIndex);

    /**
     * Destroy visual proxy and restore TireConstruct visibility
     */
    void DestroyTireVisualProxy(int32 WheelIndex);
    
    /** Calibrate suspension assembly from axle member specifications */
    void CalibrateSuspensionAssembly();

    /** Calibrate steering assembly - initialize steering parameters */
    void CalibrateSteeringAssembly();
    
    /** Calculate roll center heights from suspension hardpoint sockets
     *  
     *  Supports multiple suspension types:
     *  - Telescopic: RC at mount height (with optional correction factor)
     *  - Double Wishbone: RC from instant center geometry
     *  - MacPherson Strut: RC from strut axis intersection
     *  - Multi-Link: RC from virtual instant center
     *  - Solid Axle: RC at axle center height
     *  - Manual: Use user-specified values (no calculation)
     *  
     *  Socket naming convention:
     *  - SuspensionMount_FL/FR/RL/RR (spring/damper top mount)
     *  - UpperBallJoint_FL/FR/RL/RR (upper control arm to upright)
     *  - LowerBallJoint_FL/FR/RL/RR (lower control arm to upright)
     *  - UpperInboard_FL/FR/RL/RR (upper control arm chassis pivot)
     *  - LowerInboard_FL/FR/RL/RR (lower control arm chassis pivot)
     *  
     *  Called once during BeginPlay() - roll centers are STATIC
     */
    void CalculateRollCentersFromSockets();

    /** Precompute Pacejka tire caches for all wheels - call during initialization and when tire params change */
    void PrecomputeTireCaches();
    
    /** Apply equilibrium transform to position vehicle at correct ride height */
    void ApplyEquilibriumTransform();

    /** Initialize drivetrain system and publish to physics thread */
    void InitializeDrivetrain_GameThread();

    /** Initialize aerodynamics system and publish to physics thread */
    void InitializeAerodynamics_GameThread();

    //--------------------------------------------------------------------------
    //                          P2P REPLICATION
    //--------------------------------------------------------------------------
    // Note: UPROPERTY/UFUNCTION cannot be inside #if blocks (UHT limitation)
    // P2P logic is controlled at runtime via P2P macro in .cpp files
    //--------------------------------------------------------------------------
public:
    /** Check if this instance has physics authority (host/server) */
    FORCEINLINE bool HasPhysicsAuthority() const
    {
#if P2P
        return HasAuthority() || GetLocalRole() == ROLE_Authority;
#else
        return true; // Non-networked: always has authority
#endif
    } // End HasPhysicsAuthority()

    /** Check if this instance is a simulated proxy (client) */
    FORCEINLINE bool IsSimulatedProxy() const
    {
#if P2P
        return GetLocalRole() == ROLE_SimulatedProxy;
#else
        return false; // Non-networked: never a proxy
#endif
    } // End IsSimulatedProxy()

    //--------------------------------------------------------------------------
    //                          REPLICATED STATE
    //--------------------------------------------------------------------------

    /** Replicated vehicle state - sent from host to clients at 60Hz */
    UPROPERTY(Replicated)
    FReplicatedVehicleState ReplicatedState;

    /** Replication callback - called on clients when state is received */
    UFUNCTION()
    void OnRep_VehicleState();

    //--------------------------------------------------------------------------
    //                          CLIENT → HOST INPUT RPC
    //--------------------------------------------------------------------------

    /** Send client input to host (unreliable for low latency) */
    UFUNCTION(Server, Unreliable)
    void Server_SendInput(FReplicatedVehicleInput Input);

    //--------------------------------------------------------------------------
    //                          VEHICLE EVENTS RPC
    //--------------------------------------------------------------------------

    /** Broadcast vehicle event to all clients (reliable for important events) */
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_VehicleEvent(uint8 EventType, int32 EventData);

protected:
    //--------------------------------------------------------------------------
    //                          INTERPOLATION STATE (CLIENT-SIDE)
    //--------------------------------------------------------------------------

    /** Start state for interpolation */
    FReplicatedVehicleState InterpolationStart;

    /** Target state for interpolation */
    FReplicatedVehicleState InterpolationTarget;

    /** Current interpolation alpha [0-1] */
    float InterpolationAlpha = 0.0f;

    /** Replication timing accumulator */
    float ReplicationAccumulator = 0.0f;

    /** Replication interval (60 Hz = 16.67ms) */
    static constexpr float ReplicationInterval = 1.0f / 60.0f; // [s]

    /** Physics frame counter for state ordering */
    uint16 PhysicsFrameCounter = 0;

    /** Client input sequence counter */
    uint32 InputSequenceCounter = 0;

    //--------------------------------------------------------------------------
    //                          P2P HELPER FUNCTIONS
    //--------------------------------------------------------------------------

    /** Pack current physics state into compact replicated format */
    void PackReplicatedState(FReplicatedVehicleState& OutState);

    /** Apply received replicated state to vehicle visuals */
    void ApplyReplicatedState(const FReplicatedVehicleState& InState);

    /** Interpolate between start and target states */
    void InterpolateState(float Alpha);

}; // End AVehicleSolver
