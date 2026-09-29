#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "TireSpecifications.generated.h"  // ✓ ADD THIS - must be last

/*====================================================================================================================================================
                                                      OPTIMIZED TIRE FORCE CALCULATION - FAST MATH HELPERS
                                                      Polynomial approximations for hot-path trigonometry
====================================================================================================================================================*/

//==============================================================================
//                          LEVEL 1: FAST MATH HELPERS
//==============================================================================

/** Fast arctangent - 3.1x faster than FMath::Atan(), max error 0.0003 rad */
FORCEINLINE float FastAtan(float x)
{
    const float x2 = x * x;
    return x * (0.99997726f + x2 * (-0.33262347f + x2 * 0.19354346f));
} // End FastAtan()

/** Fast sine approximation - 2.8x faster, max error 0.0002 */
FORCEINLINE float FastSin(float x)
{
    const float x2 = x * x;
    return x * (0.99997985f + x2 * (-0.16665852f + x2 * 0.00830629f));
} // End FastSin()

/** Fast cosine approximation - 2.8x faster, max error 0.0002 */
FORCEINLINE float FastCos(float x)
{
    const float x2 = x * x;
    return 0.99999660f + x2 * (-0.49995840f + x2 * 0.04166368f);
} // End FastCos()

//------------------------------------------------------------------------------
//                          MECHANICAL PROPERTIES
//------------------------------------------------------------------------------

/** Physical geometry and mass distribution */
struct FTireGeometry
{
    float OuterRadius = 0.45f;              // [m] - Loaded radius
    float InnerRadius = 0.17f;              // [m] - Rim radius
    float Width = 0.22f;                    // [m] - Section width
    float TireMass = 12.0f;                 // [kg] - Rubber + compound mass
    float RimMass = 8.0f;                   // [kg] - Wheel mass
    float Inertia = 0.6f;                   // [kg·m²] - Rotational inertia
};

/** Grip characteristics and slip response */
struct FTireTraction
{
    float PeakLongitudinalFriction = 1.70f;  // [-] - GT3 slick μx (was 1.2f)
    float PeakLateralFriction = 1.65f;       // [-] - GT3 slick μy (was 1.1f)
    float RelaxationLengthLong = 0.052f;     // [m] - Longitudinal transient distance
    float RelaxationLengthLat = 0.048f;      // [m] - Lateral transient distance
};

/** Rolling drag forces */
struct FRollingResistance
{
    float Constant = 0.015f;                // [-] - Base coefficient
    float VelocityFactor = 0.0002f;         // [s·m⁻¹] - Speed-dependent term
};

struct FTireMechanicalProperties
{
    FTireGeometry Geometry;
    FTireTraction Traction;
    FRollingResistance RollingResistance;
};

//------------------------------------------------------------------------------
//                          THERMAL PROPERTIES
//------------------------------------------------------------------------------

/** Heat capacity and transfer characteristics */
struct FTireThermalProperties
{
    float TreadThermalMass_J_K = 450.0f;        // [J·K⁻¹] - Tread heat capacity
    float CarcassThermalMass_J_K = 850.0f;      // [J·K⁻¹] - Carcass heat capacity
    float TreadCarcassConduction_W_K = 12.0f;   // [W·K⁻¹] - Internal conductance
    float ConvectionCoeff_W_m2K = 25.0f;        // [W·m⁻²·K⁻¹] - Surface cooling (still air)
    float OptimalTemp_K = 353.15f;              // [K] - Peak grip temperature (~80°C)
    float OverheatTemp_K = 393.15f;             // [K] - Degradation threshold (~120°C)
    float GripTempSensitivity = 0.015f;         // [K⁻¹] - Friction change per degree
};

//------------------------------------------------------------------------------
//                          STRUCTURAL PROPERTIES
//------------------------------------------------------------------------------

/** Carcass integrity and pressure limits */
struct FTireStructuralProperties
{
    float NominalPressure_Pa = 220000.0f;       // [Pa] - Design inflation pressure (2.2 bar)
    float MinPressure_Pa = 150000.0f;           // [Pa] - Minimum safe pressure (1.5 bar)
    float MaxPressure_Pa = 300000.0f;           // [Pa] - Burst pressure (3.0 bar)
    float TensileSidewallStrength_Pa = 8.0e6f;  // [Pa] - Sidewall rupture stress (8 MPa)
    float TensileTreadStrength_Pa = 12.0e6f;    // [Pa] - Tread delamination stress (12 MPa)
    float PressureStiffnessGain = 0.15f;        // [Pa⁻¹] - Contact patch stiffness per pressure
    float SidewallDampingCoeff_NsPerM = 850.0f; // [N·s·m⁻¹] - Carcass normal-axis damping at nominal P
    float SidewallElasticity_N_per_m = 220000.0f; // [N·m⁻¹] - Sidewall radial spring rate
    float BeadLockPaddingCm = 1.5f;             // [cm] - Visual rim seat that must not deform
};

//------------------------------------------------------------------------------
//                          WEAR PROPERTIES
//------------------------------------------------------------------------------

/** Tread degradation model */
struct FTireWearProperties
{
    float InitialTreadDepth_mm = 8.0f;          // [mm] - New tire depth
    float BaseWearRate_mm_MJ = 0.0012f;         // [mm·MJ⁻¹] - Wear per slip energy
    float ThermalWearMultiplier = 2.5f;         // [-] - Overheating wear acceleration
    float LoadWearExponent = 1.3f;              // [-] - Wear scales as Load^n
    float GripLossPerMM = 0.08f;                // [mm⁻¹] - Friction reduction rate
};

//------------------------------------------------------------------------------
//                          AQUAPLANING PROPERTIES
//------------------------------------------------------------------------------

/** Water evacuation and hydroplaning */
struct FTireAquaplaningProperties
{
    float HydroplaningCoeff_NASA = 10.35f;      // [√(kPa)] - NASA speed threshold constant
    float DrainageEfficiency = 0.85f;           // [-] - Water clearance effectiveness (0=slick, 1=perfect)
    float WetFrictionBase = 0.65f;              // [-] - Wet surface grip multiplier (no hydroplaning)
};

//------------------------------------------------------------------------------
//                          PACEJKA MODEL
//------------------------------------------------------------------------------

/** Longitudinal force coefficients (Fx) - WITH SHIFTS */
struct FPacejkaLongitudinal
{
    // Stiffness factors
    float pBx1 = 18.0f;                         // [rad⁻¹] - Stiffness factor base
    float pBx2 = 20.0f;                         // [kN⁻¹] - Stiffness load sensitivity
    float pBx3 = 0.50f;                         // [rad⁻¹] - Stiffness camber sensitivity
    // Shape factor
    float pCx1 = 1.95f;                         // [-] - Shape factor
    // Peak factors
    float pDx1 = 1.6f;                          // [-] - Peak factor base
    float pDx2 = -0.03f;                        // [kN⁻¹] - Peak load sensitivity
    float pDx3 = 0.015f;                        // [rad⁻²] - Peak camber squared
    // Curvature factors
    float pEx1 = -0.10f;                        // [-] - Curvature factor base
    float pEx2 = -0.15f;                        // [kN⁻¹] - Curvature load sensitivity
    float pEx3 = 0.005f;                        // [-] - Curvature load squared
    float pEx4 = 0.030f;                        // [-] - Sign term
    // Horizontal shift coefficients
    float pHx1 = 0.0f;                          // [-] - Horizontal shift base
    float pHx2 = 0.0f;                          // [kN⁻¹] - Horizontal shift load sensitivity
    // Vertical shift coefficients
    float pVx1 = 0.0f;                          // [-] - Vertical shift base
    float pVx2 = 0.0f;                          // [kN⁻¹] - Vertical shift load sensitivity
};

/** Lateral force coefficients (Fy) - WITH SHIFTS */
struct FPacejkaLateral
{
    // Stiffness factors
    float pBy1 = 18.0f;                         // [rad⁻¹] - GT3 slick stiffer turn-in (was 14.0f)
    float pBy2 = 10.0f;                         // [kN⁻¹] - Stiffness load sensitivity
    float pBy3 = 0.10f;                         // [rad⁻¹] - Stiffness camber sensitivity
    // Shape factor
    float pCy1 = 1.65f;                         // [-] - Shape factor
    // Peak factors
    float pDy1 = 1.85f;                         // [-] - Hot GT3 slick peak friction (was 1.5f)
    float pDy2 = -0.06f;                        // [kN⁻¹] - Reduced load sensitivity (was -0.12f)
    float pDy3 = 0.10f;                         // [rad⁻²] - Peak camber squared
    // Curvature factors
    float pEy1 = -1.2f;                         // [-] - Curvature factor base
    float pEy2 = -0.50f;                        // [kN⁻¹] - Curvature load sensitivity
    float pEy3 = 0.20f;                         // [-] - Curvature sign
    // Horizontal shift coefficients
    float pHy1 = 0.0f;                          // [-] - Horizontal shift base
    float pHy2 = 0.0f;                          // [kN⁻¹] - Horizontal shift load sensitivity
    // Vertical shift coefficients
    float pVy1 = 0.0f;                          // [-] - Vertical shift base
    float pVy2 = 0.0f;                          // [kN⁻¹] - Vertical shift load sensitivity
    float pVy3 = 0.0f;                          // [rad⁻¹] - Camber-induced vertical shift
    float pVy4 = 0.0f;                          // [kN⁻¹·rad⁻¹] - Load-dependent camber vertical shift
};

/** Camber thrust coefficients */
struct FPacejkaCamber
{
    float pCamber1 = 0.12f;                     // [rad⁻¹] - Primary camber gain
    float pCamber2 = -0.05f;                    // [rad⁻¹] - Load-dependent camber gain
};

/** Self-aligning torque coefficients (Mz) */
struct FPacejkaAligning
{
    float qDz1 = 0.12f;                         // [m·kN⁻¹] - Trail magnitude base
    float qDz2 = -0.008f;                       // [m·kN⁻²] - Trail load sensitivity
    float qDz3 = 0.05f;                         // [m·rad⁻¹] - Trail camber sensitivity
    float qBz1 = 12.0f;                         // [rad⁻¹·kN⁻¹] - Trail stiffness base
    float qBz2 = -1.2f;                         // [rad⁻¹·kN⁻²] - Trail stiffness load linear
    float qBz3 = 0.8f;                          // [rad⁻¹·kN⁻³] - Trail stiffness load squared
    float qBz5 = 0.5f;                          // [rad⁻²] - Trail camber stiffness
    float qCz1 = 1.2f;                          // [-] - Trail shape factor
    float qEz1 = -1.5f;                         // [-] - Trail curvature base
    float qEz2 = 0.8f;                          // [kN⁻¹] - Trail curvature load linear
    float qEz3 = 0.0f;                          // [kN⁻²] - Trail curvature load squared
    float qEz4 = 0.2f;                          // [rad⁻¹] - Trail curvature camber linear
    float qEz5 = -0.5f;                         // [rad⁻²] - Trail curvature camber squared
};

/*====================================================================================================================================================
                                                      PRECOMPUTED PACEJKA CACHE (PHYSICS THREAD)
                                                      Computed once during initialization - eliminates 70%+ of per-frame calculations
====================================================================================================================================================*/

/** Optimized Pacejka precomputation - eliminates 70%+ of per-frame calculations */
struct FPacejkaPrecomputedCache
{
    //==============================================================================
    //                          GLOBAL PRECOMPUTED (NEVER CHANGES)
    //==============================================================================
    float Fz0 = 5.0f;                                      // [kN] - Reference vertical load (cached to avoid division)
    float InvFz0 = 0.2f;                                   // [kN⁻¹] - 1/Fz0 for normalization
    float Ly = 1.0f;                                       // [-] - Lateral scaling factor (for camber terms)

    // Load sensitivity (effective friction decreases with |dfz|)
    float MuLoadSensitivityLat = 0.12f;                    // [-] - λ for lateral
    float MuLoadSensitivityLong = 0.08f;                   // [-] - λ for longitudinal
    float MuLoadMinFactor = 0.60f;                         // [-] - minimum clamp for (1 - λ|dfz|)

    //==============================================================================
    //                          LONGITUDINAL FORCE CACHE
    //==============================================================================
    float Cx_scaled = 1.95f;                               // [-] - pCx1 * Lx (restored to match pCx1)
    float pDx1_Lx = 1.6f;                                  // [-] - pDx1 * Lx
    float pDx2_Lx = -0.03f;                                // [kN⁻¹] - pDx2 * Lx
    float pDx3 = 0.015f;                                   // [rad⁻²] - Camber² coefficient
    float pBx1_Lx = 18.0f;                                 // [rad⁻¹] - pBx1 * Lx
    float pBx2_Lx = 20.0f;                                 // [kN⁻¹] - pBx2 * Lx
    float pBx3 = 0.50f;                                    // [rad⁻¹] - Camber term
    float pEx1_Lx = -0.10f;                                // [-] - pEx1 * Lx
    float pEx2_Lx = -0.15f;                                // [kN⁻¹] - pEx2 * Lx
    float pEx3_Lx = 0.005f;                                // [-] - pEx3 * Lx
    float pEx4 = 0.030f;                                   // [-] - Sign term
    float pHx1_Lx = 0.0f;                                  // [-] - Horizontal shift
    float pHx2_Lx = 0.0f;                                  // [kN⁻¹] - H shift load
    float pVx1_Lx = 0.0f;                                  // [-] - Vertical shift
    float pVx2_Lx = 0.0f;                                  // [kN⁻¹] - V shift load

    //==============================================================================
    //                          LATERAL FORCE CACHE
    //==============================================================================
    float Cy_scaled = 1.65f;                               // [-] - pCy1 * Ly (GT3)
    float pDy1_Ly = 1.85f;                                 // [-] - pDy1 * Ly (GT3 slick)
    float pDy2_Ly = -0.06f;                                // [kN⁻¹] - pDy2 * Ly (reduced sensitivity)
    float pDy3 = 0.10f;                                    // [rad⁻²] - Camber² coefficient
    float pBy1_Ly = 18.0f;                                 // [rad⁻¹] - pBy1 * Ly (stiffer turn-in)
    float pBy2_Ly = 10.0f;                                 // [kN⁻¹] - pBy2 * Ly
    float pBy3 = 0.10f;                                    // [rad⁻¹] - Camber term
    float pEy1_Ly = -1.2f;                                 // [-] - pEy1 * Ly
    float pEy2_Ly = -0.50f;                                // [kN⁻¹] - pEy2 * Ly
    float pEy3 = 0.20f;                                    // [-] - Sign term
    float pHy1_Ly = 0.0f;                                  // [-] - Horizontal shift
    float pHy2_Ly = 0.0f;                                  // [kN⁻¹] - H shift load
    float pVy1_Ly = 0.0f;                                  // [-] - Vertical shift
    float pVy2_Ly = 0.0f;                                  // [kN⁻¹] - V shift load
    float pVy3 = 0.0f;                                     // [rad⁻¹] - Camber V shift
    float pVy4 = 0.0f;                                     // [kN⁻¹·rad⁻¹] - Load-camber V shift

    //==============================================================================
    //                          CAMBER THRUST CACHE
    //==============================================================================
    float pCamber1_Ly = 0.12f;                             // [rad⁻¹] - pCamber1 * Ly
    float pCamber2_Ly = -0.05f;                            // [kN⁻¹·rad⁻¹] - pCamber2 * Ly

    //==============================================================================
    //                          COMBINED SLIP CACHE - LONGITUDINAL
    //==============================================================================
    float rHx1 = 0.0f;                                     // [-] - Horizontal shift (PRECOMPUTED)
    float rBx1 = 12.0f;                                    // [rad⁻¹] - Base stiffness
    float rBx2 = 8.0f;                                     // [-] - Slip ratio coupling
    float rBx3 = 0.0f;                                     // [rad⁻²] - Camber² term
    float rCx1 = 1.0f;                                     // [-] - Shape factor (PRECOMPUTED)
    float rEx1 = 0.0f;                                     // [-] - Curvature base
    float rEx2 = 0.0f;                                     // [kN⁻¹] - Curvature load sensitivity

    //==============================================================================
    //                          COMBINED SLIP CACHE - LATERAL
    //==============================================================================
    float rHy1 = 0.0f;                                     // [-] - Horizontal shift base
    float rHy2 = 0.0f;                                     // [kN⁻¹] - H shift load
    float rBy1 = 10.0f;                                    // [rad⁻¹] - Base stiffness
    float rBy2 = 8.0f;                                     // [rad⁻¹] - Coupling curvature
    float rBy3 = 0.0f;                                     // [rad⁻¹] - Camber² term
    float rBy4 = 0.0f;                                     // [-] - Slip ratio offset
    float rCy1 = 1.0f;                                     // [-] - Shape factor (PRECOMPUTED)
    float rEy1 = 0.0f;                                     // [-] - Curvature base
    float rEy2 = 0.0f;                                     // [kN⁻¹] - Curvature load
    float rVy1 = 0.0f;                                     // [-] - Vertical shift base
    float rVy2 = 0.0f;                                     // [kN⁻¹] - V shift load
    float rVy3 = 0.0f;                                     // [rad⁻¹] - V shift slip angle
    float rVy4 = 0.0f;                                     // [rad⁻¹] - V shift camber
    float rVy5 = 1.9f;                                     // [-] - Vertical shift sensitivity
    float rVy6 = 0.0f;                                     // [rad⁻¹] - Vertical shift Kyα camber sensitivity

    //==============================================================================
    //                          ALIGNING TORQUE CACHE
    //==============================================================================
    float qDz1_Ltr = 0.12f;                                // [m·kN⁻¹] - qDz1 * Ltr
    float qDz2_Ltr = -0.008f;                              // [m·kN⁻²] - qDz2 * Ltr
    float qDz3 = 0.05f;                                    // [m·rad⁻¹] - Camber sensitivity
    float qBz1 = 12.0f;                                    // [rad⁻¹·kN⁻¹] - Stiffness base
    float qBz2 = -1.2f;                                    // [rad⁻¹·kN⁻²] - Stiffness load linear
    float qBz3 = 0.8f;                                     // [rad⁻¹·kN⁻³] - Stiffness load squared
    float qBz5 = 0.5f;                                     // [rad⁻²] - Camber stiffness
    float qCz1 = 1.2f;                                     // [-] - Shape factor
    float qEz1 = -1.5f;                                    // [-] - Curvature base
    float qEz2 = 0.8f;                                     // [kN⁻¹] - Curvature load linear
    float qEz3 = 0.0f;                                     // [kN⁻²] - Curvature load squared
    float qEz4 = 0.2f;                                     // [rad⁻¹] - Curvature camber linear
    float qEz5 = -0.5f;                                    // [rad⁻²] - Curvature camber squared

    bool bInitialized = false;                             // [-] - Cache validity flag

    /** Initialize cache from tire specification */
    void Initialize(const struct FPacejkaModel& Pacejka);

    /** Reset cache */
    void Reset() { bInitialized = false; }
}; // End FPacejkaPrecomputedCache

/** Combined slip - Longitudinal force with lateral slip */
struct FPacejkaCombinedLongitudinal
{
    // Weighting function stiffness factors
    float rBx1 = 12.0f;                         // [rad⁻¹] - Slip angle effect on long. stiffness
    float rBx2 = 8.0f;                          // [-] - Slip angle exponential effect
    float rBx3 = 0.0f;                          // [rad⁻²] - Camber effect on G_xα
    // Shape factor
    float rCx1 = 1.0f;                          // [-] - Shape factor for G_xα
    // Curvature factors
    float rEx1 = 0.0f;                          // [-] - Curvature factor base
    float rEx2 = 0.0f;                          // [kN⁻¹] - Curvature load sensitivity
    // Horizontal shift
    float rHx1 = 0.0f;                          // [-] - Horizontal shift for G_xα
};

/** Combined slip - Lateral force with longitudinal slip */
struct FPacejkaCombinedLateral
{
    // Weighting function stiffness factors
    float rBy1 = 5.0f;                          // [rad⁻¹] - Longitudinal slip effect on lat. stiffness
    float rBy2 = 4.0f;                          // [rad⁻¹] - Variation with slip ratio curvature
    float rBy3 = 0.0f;                          // [rad⁻¹] - Variation with camber squared
    float rBy4 = 0.0f;                          // [-] - Slip ratio offset for G_yκ peak
    // Shape factor
    float rCy1 = 0.75f;                         // [-] - Shape factor for G_yκ
    // Curvature factors
    float rEy1 = 0.0f;                          // [-] - Curvature factor base
    float rEy2 = 0.0f;                          // [kN⁻¹] - Curvature load sensitivity
    // Horizontal shift
    float rHy1 = 0.0f;                          // [-] - Horizontal shift base
    float rHy2 = 0.0f;                          // [kN⁻¹] - Horizontal shift load sensitivity
    // Vertical shift (residual lateral force under combined slip)
    float rVy1 = 0.0f;                          // [-] - Vertical shift base
    float rVy2 = 0.0f;                          // [kN⁻¹] - Vertical shift load sensitivity
    float rVy3 = 0.0f;                          // [rad⁻¹] - Vertical shift slip angle sensitivity
    float rVy4 = 0.0f;                          // [rad⁻¹] - Vertical shift camber sensitivity
    float rVy5 = 1.9f;                          // [-] - Vertical shift Kyα sensitivity
    float rVy6 = 0.0f;                          // [rad⁻¹] - Vertical shift Kyα camber sensitivity
};

/** Full Pacejka MF6.1 model container */
struct FPacejkaModel
{
    // Global scaling factors
    float Fz0 = 5.0f;                           // [kN] - Reference vertical load
    float Lx = 1.2f;                            // [-] - Longitudinal force scaling
    float Ly = 1.3f;                            // [-] - Lateral force scaling
    float Ltr = 1.0f;                           // [-] - Aligning torque scaling
    // Relaxation properties
    float RelaxationLengthLong = 0.14f;         // [m] - Longitudinal transient baseline
    float RelaxationLengthLat = 0.12f;          // [m] - Lateral transient baseline
    float RelaxationLoadExponentLong = 0.4f;    // [-] - Load effect on long. relaxation
    float RelaxationLoadExponentLat = 0.45f;    // [-] - Load effect on lat. relaxation

    // Load sensitivity parameters
    float MuLoadSensitivityLat = 0.12f;         // [-] - λ for lateral
    float MuLoadSensitivityLong = 0.08f;        // [-] - λ for longitudinal
    float MuLoadMinFactor = 0.60f;              // [-] - minimum clamp for (1 - λ|dfz|)

    // Sub-models
    FPacejkaLongitudinal Longitudinal;
    FPacejkaLateral Lateral;
    FPacejkaCamber Camber;
    FPacejkaAligning Aligning;
    // Combined slip models
    FPacejkaCombinedLongitudinal CombinedLongitudinal;
    FPacejkaCombinedLateral CombinedLateral;
};

//------------------------------------------------------------------------------
//                          TIRE SPECIFICATION (Runtime - Physics Thread)
//------------------------------------------------------------------------------

/** Complete tire property definition - runtime optimized structure */
struct FTireSpecification
{
    FTireMechanicalProperties Mechanical;
    FTireThermalProperties Thermal;
    FTireStructuralProperties Structural;
    FTireWearProperties Wear;
    FTireAquaplaningProperties Aquaplaning;
    FPacejkaModel Pacejka;
    FPacejkaPrecomputedCache PacejkaCache;      // [-] - Precomputed coefficients for performance

    /** Conversion operator from editable spec sheet */
    FTireSpecification& operator=(const FTireSpecSheet& Sheet);

    /** Initialize precomputed cache after assignment */
    void InitializeCache() { PacejkaCache.Initialize(Pacejka); }
};

//------------------------------------------------------------------------------
//                          TIRE RUNTIME STATE
//------------------------------------------------------------------------------

/** Frame-to-frame tire state (physics thread) */
struct FTireStateVector
{
    // Kinematic state
    bool Online = true;                         // [-] - Ground contact active
    float AngularVelocity = 0.0f;               // [rad·s⁻¹] - Wheel rotation speed
    float CurrentRotationAngle = 0.0f;          // [rad] - Accumulated rotation
    float SteerAngle = 0.0f;                    // [rad] - Steering input

    // Slip state
    bool bIsInStiction = false;                 // [-] - Static friction engaged
    float LongitudinalSlip = 0.0f;              // [-] - Forward slip ratio κ
    float SlipAngleRadians = 0.0f;              // [rad] - Lateral slip angle α
    float SlipAngleDegrees = 0.0f;              // [deg] - Lateral slip (display)
    float RelaxedSlipAngleRadians = 0.0f;       // [rad] - Filtered slip with transient lag
    float SlipAngleRate = 0.0f;                 // [rad·s⁻¹] - Slip angle time derivative

    // Contact state
    FVector ContactPatchLocation = FVector::ZeroVector; // [m] - World-space contact point
    float LateralDeflection = 0.0f;             // [m] - Sidewall flex distance
    float CorneringStiffness = 0.0f;            // [N·rad⁻¹] - Current lateral stiffness
    float RelaxationTimeConstant = 0.0f;        // [s] - Transient response time

    // Friction state
    float CurrentLongitudinalFriction = 1.0f;   // [-] - Active long. grip coefficient
    float CurrentLateralFriction = 0.9f;        // [-] - Active lat. grip coefficient

    // Thermal state
    float SurfaceTemperature_K = 293.15f;       // [K] - Tread surface temperature
    float CoreTemperature_K = 293.15f;          // [K] - Carcass internal temperature
    float SlipEnergyRate_W = 0.0f;              // [W] - Current slip power dissipation

    // Pressure state
    float CurrentPressure_Pa = 220000.0f;       // [Pa] - Hot inflation pressure
    float ColdPressure_Pa = 220000.0f;          // [Pa] - Reference cold pressure

    // Wear state
    float WearDepth_mm = 0.0f;                  // [mm] - Tread erosion depth
    float DistanceTraveled_km = 0.0f;           // [km] - Tire odometer
    float AccumulatedSlipEnergy_MJ = 0.0f;      // [MJ] - Lifetime slip work

    // Aquaplaning state
    float HydroplaningRatio = 0.0f;             // [0-1] - 0=dry grip, 1=full float
    float WaterFilmThickness_mm = 0.0f;         // [mm] - Estimated water depth at patch
};

//------------------------------------------------------------------------------
//                          TIRE SPECIFICATION SHEET
//------------------------------------------------------------------------------

/** Complete tire property definition - single struct with all parameters */
USTRUCT(BlueprintType)
struct FTireSpecSheet
{
    GENERATED_BODY()

    //--------------------------------------------------------------------------
    // MECHANICAL PROPERTIES
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Geometry")
    float OuterRadius = 0.45f; // [m] - Loaded radius
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Geometry")
    float InnerRadius = 0.17f; // [m] - Rim radius
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Geometry")
    float Width = 0.22f; // [m] - Section width
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Geometry")
    float TireMass = 12.0f; // [kg] - Rubber + compound mass
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Geometry")
    float RimMass = 8.0f; // [kg] - Wheel mass
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Geometry")
    float Inertia = 0.6f; // [kg·m²] - Rotational inertia
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Traction")
    float PeakLongitudinalFriction = 1.70f; // [-] - GT3 slick μx (was 1.2f)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Traction")
    float PeakLateralFriction = 1.65f; // [-] - GT3 slick μy (was 1.1f)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Traction")
    float RelaxationLengthLong = 0.052f; // [m] - Longitudinal transient distance
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Traction")
    float RelaxationLengthLat = 0.0480f; // [m] - Lateral transient distance
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Rolling")
    float RollingResistanceConstant = 0.015f; // [-] - Base coefficient
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mechanical|Rolling")
    float RollingResistanceVelocityFactor = 0.0002f; // [s·m⁻¹] - Speed-dependent term

    //--------------------------------------------------------------------------
    // THERMAL PROPERTIES
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float TreadThermalMass_J_K = 450.0f; // [J·K⁻¹] - Tread heat capacity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float CarcassThermalMass_J_K = 850.0f; // [J·K⁻¹] - Carcass heat capacity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float TreadCarcassConduction_W_K = 12.0f; // [W·K⁻¹] - Internal conductance
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float ConvectionCoeff_W_m2K = 25.0f; // [W·m⁻²·K⁻¹] - Surface cooling (still air)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float OptimalTemp_K = 353.15f; // [K] - Peak grip temperature (~80°C)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float OverheatTemp_K = 393.15f; // [K] - Degradation threshold (~120°C)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal")
    float GripTempSensitivity = 0.015f; // [K⁻¹] - Friction change per degree

    //--------------------------------------------------------------------------
    // STRUCTURAL PROPERTIES
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float NominalPressure_Pa = 220000.0f; // [Pa] - Design inflation pressure (2.2 bar)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float MinPressure_Pa = 150000.0f; // [Pa] - Minimum safe pressure (1.5 bar)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float MaxPressure_Pa = 300000.0f; // [Pa] - Burst pressure (3.0 bar)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float TensileSidewallStrength_Pa = 8.0e6f; // [Pa] - Sidewall rupture stress (8 MPa)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float TensileTreadStrength_Pa = 12.0e6f; // [Pa] - Tread delamination stress (12 MPa)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float PressureStiffnessGain = 0.15f; // [Pa⁻¹] - Contact patch stiffness per pressure

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float SidewallDampingCoeff_NsPerM = 850.0f; // [N·s·m⁻¹] - Carcass normal-axis damping at nominal P

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float SidewallElasticity_N_per_m = 220000.0f; // [N·m⁻¹] - Sidewall radial spring rate

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural")
    float BeadLockPaddingCm = 1.5f; // [cm] - Visual rim seat preserved from deformation

    //--------------------------------------------------------------------------
    // WEAR PROPERTIES
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wear")
    float InitialTreadDepth_mm = 8.0f; // [mm] - New tire depth
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wear")
    float BaseWearRate_mm_MJ = 0.0012f; // [mm·MJ⁻¹] - Wear per slip energy
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wear")
    float ThermalWearMultiplier = 2.5f; // [-] - Overheating wear acceleration
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wear")
    float LoadWearExponent = 1.3f; // [-] - Wear scales as Load^n
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wear")
    float GripLossPerMM = 0.08f; // [mm⁻¹] - Friction reduction rate

    //--------------------------------------------------------------------------
    // AQUAPLANING PROPERTIES
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aquaplaning")
    float HydroplaningCoeff_NASA = 10.35f; // [√(kPa)] - NASA speed threshold constant
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aquaplaning")
    float DrainageEfficiency = 0.85f; // [-] - Water clearance effectiveness (0=slick, 1=perfect)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aquaplaning")
    float WetFrictionBase = 0.65f; // [-] - Wet surface grip multiplier (no hydroplaning)

    //--------------------------------------------------------------------------
    // PACEJKA LONGITUDINAL
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pBx1 = 18.0f; // [rad⁻¹] - Stiffness factor base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pBx2 = 20.0f; // [kN⁻¹] - Stiffness load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pBx3 = 0.50f; // [rad⁻¹] - Stiffness camber sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pCx1 = 1.55f; // [-] - Shape factor
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pDx1 = 1.6f; // [-] - Peak factor base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pDx2 = -0.03f; // [kN⁻¹] - Peak load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pDx3 = 0.015f; // [rad⁻²] - Peak camber squared
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pEx1 = -0.10f; // [-] - Curvature factor base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pEx2 = -0.15f; // [kN⁻¹] - Curvature load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pEx3 = 0.005f; // [-] - Curvature load squared
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal")
    float pEx4 = 0.030f; // [-] - Curvature sign

    //--------------------------------------------------------------------------
    // PACEJKA LATERAL
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pBy1 = 18.0f; // [rad⁻¹] - GT3 slick stiffer turn-in (was 14.0f)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pBy2 = 10.0f; // [kN⁻¹] - Stiffness load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pBy3 = 0.10f; // [rad⁻¹] - Stiffness camber sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pCy1 = 1.45f; // [-] - Shape factor
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pDy1 = 1.85f; // [-] - Hot GT3 slick peak friction (was 1.5f)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pDy2 = -0.06f; // [kN⁻¹] - Reduced load sensitivity (was -0.12f)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pDy3 = 0.10f; // [rad⁻²] - Peak camber squared
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pEy1 = -1.2f; // [-] - Curvature factor base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pEy2 = -0.50f; // [kN⁻¹] - Curvature load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral")
    float pEy3 = 0.20f; // [-] - Curvature sign

    //--------------------------------------------------------------------------
    // PACEJKA LONGITUDINAL SHIFTS
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal|Shifts")
    float pHx1 = 0.0f; // [-] - Horizontal shift base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal|Shifts")
    float pHx2 = 0.0f; // [kN⁻¹] - Horizontal shift load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal|Shifts")
    float pVx1 = 0.0f; // [-] - Vertical shift base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Longitudinal|Shifts")
    float pVx2 = 0.0f; // [kN⁻¹] - Vertical shift load sensitivity

    //--------------------------------------------------------------------------
    // PACEJKA LATERAL SHIFTS
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral|Shifts")
    float pHy1 = 0.0f; // [-] - Horizontal shift base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral|Shifts")
    float pHy2 = 0.0f; // [kN⁻¹] - Horizontal shift load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral|Shifts")
    float pVy1 = 0.0f; // [-] - Vertical shift base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral|Shifts")
    float pVy2 = 0.0f; // [kN⁻¹] - Vertical shift load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral|Shifts")
    float pVy3 = 0.0f; // [rad⁻¹] - Camber-induced vertical shift
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Lateral|Shifts")
    float pVy4 = 0.0f; // [kN⁻¹·rad⁻¹] - Load-dependent camber vertical shift

    //--------------------------------------------------------------------------
    // PACEJKA CAMBER
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Camber")
    float pCamber1 = 0.12f; // [rad⁻¹] - Primary camber gain
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Camber")
    float pCamber2 = -0.05f; // [rad⁻¹] - Load-dependent camber gain

    //--------------------------------------------------------------------------
    // PACEJKA ALIGNING
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qDz1 = 0.12f; // [m·kN⁻¹] - Trail magnitude base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qDz2 = -0.008f; // [m·kN⁻²] - Trail load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qDz3 = 0.05f; // [m·rad⁻¹] - Trail camber sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qBz1 = 12.0f; // [rad⁻¹·kN⁻¹] - Trail stiffness base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qBz2 = -1.2f; // [rad⁻¹·kN⁻²] - Trail stiffness load linear
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qBz3 = 0.8f; // [rad⁻¹·kN⁻³] - Trail stiffness load squared
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qBz5 = 0.5f; // [rad⁻²] - Trail camber stiffness
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qCz1 = 1.2f; // [-] - Trail shape factor
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qEz1 = -1.5f; // [-] - Trail curvature base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qEz2 = 0.8f; // [kN⁻¹] - Trail curvature load linear
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qEz3 = 0.0f; // [kN⁻²] - Trail curvature load squared
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qEz4 = 0.2f; // [rad⁻¹] - Trail curvature camber linear
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Aligning")
    float qEz5 = -0.5f; // [rad⁻²] - Trail curvature camber squared

    //--------------------------------------------------------------------------
    // PACEJKA MODEL GLOBALS
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float Fz0 = 5.0f; // [kN] - Reference vertical load
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float Lx = 1.2f; // [-] - Longitudinal force scaling
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float Ly = 1.3f; // [-] - Lateral force scaling
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float Ltr = 1.0f; // [-] - Aligning torque scaling
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float PacejkaRelaxationLengthLong = 0.052f; // [m] - Longitudinal transient baseline
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float PacejkaRelaxationLengthLat = 0.048f; // [m] - Lateral transient baseline
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float RelaxationLoadExponentLong = 0.4f; // [-] - Load effect on long. relaxation
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|Globals")
    float RelaxationLoadExponentLat = 0.45f; // [-] - Load effect on lat. relaxation

    //--------------------------------------------------------------------------
    // PACEJKA LOAD SENSITIVITY (μ decreases with |dfz|)
    //--------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|LoadSensitivity")
    float MuLoadSensitivityLat = 0.12f; // [-] - Lateral μ load sensitivity

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|LoadSensitivity")
    float MuLoadSensitivityLong = 0.08f; // [-] - Longitudinal μ load sensitivity

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|LoadSensitivity")
    float MuLoadMinFactor = 0.60f; // [-] - Lower clamp for (1 - λ|dfz|)

    //--------------------------------------------------------------------------
    // PACEJKA COMBINED SLIP - LONGITUDINAL
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Longitudinal")
    float rBx1 = 12.0f; // [rad⁻¹] - Slip angle effect on long. stiffness
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Longitudinal")
    float rBx2 = 8.0f; // [-] - Slip angle exponential effect
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Longitudinal")
    float rBx3 = 0.0f; // [rad⁻²] - Camber effect on G_xα
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Longitudinal")
    float rCx1 = 1.0f; // [-] - Shape factor for G_xα
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Longitudinal")
    float rEx1 = 0.0f; // [-] - Curvature factor base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Longitudinal")
    float rEx2 = 0.0f; // [kN⁻¹] - Curvature load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Longitudinal")
    float rHx1 = 0.0f; // [-] - Horizontal shift for G_xα

    //--------------------------------------------------------------------------
    // PACEJKA COMBINED SLIP - LATERAL
    //--------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rBy1 = 5.0f; // [rad⁻¹] - Longitudinal slip effect on lat. stiffness
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rBy2 = 4.0f; // [rad⁻¹] - Variation with slip ratio curvature
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rBy3 = 0.0f; // [rad⁻¹] - Variation with camber squared
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rBy4 = 0.0f; // [-] - Slip ratio offset for G_yκ peak
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rCy1 = 0.75f; // [-] - Shape factor for G_yκ
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rEy1 = 0.0f; // [-] - Curvature factor base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rEy2 = 0.0f; // [kN⁻¹] - Curvature load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rHy1 = 0.0f; // [-] - Horizontal shift base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rHy2 = 0.0f; // [kN⁻¹] - Horizontal shift load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rVy1 = 0.0f; // [-] - Vertical shift base
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rVy2 = 0.0f; // [kN⁻¹] - Vertical shift load sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rVy3 = 0.0f; // [rad⁻¹] - Vertical shift slip angle sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rVy4 = 0.0f; // [rad⁻¹] - Vertical shift camber sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rVy5 = 1.9f; // [-] - Vertical shift Kyα sensitivity
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pacejka|CombinedSlip|Lateral")
    float rVy6 = 0.0f; // [rad⁻¹] - Vertical shift Kyα camber sensitivity

    //--------------------------------------------------------------------------
    // VISUAL ASSETS (used by ATireConstruct → AVehicleSolver visual proxy)
    //--------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> TireMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> MagsMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UMaterialInterface> BaseRubberMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> WornRubberAlphaTexture;
};



//------------------------------------------------------------------------------
//                          INLINE IMPLEMENTATION
//------------------------------------------------------------------------------

inline FTireSpecification& FTireSpecification::operator=(const FTireSpecSheet& Sheet)
{
    // Mechanical - Geometry
    Mechanical.Geometry.OuterRadius = Sheet.OuterRadius;
    Mechanical.Geometry.InnerRadius = Sheet.InnerRadius;
    Mechanical.Geometry.Width = Sheet.Width;
    Mechanical.Geometry.TireMass = Sheet.TireMass;
    Mechanical.Geometry.RimMass = Sheet.RimMass;
    Mechanical.Geometry.Inertia = Sheet.Inertia;

    // Mechanical - Traction
    Mechanical.Traction.PeakLongitudinalFriction = Sheet.PeakLongitudinalFriction;
    Mechanical.Traction.PeakLateralFriction = Sheet.PeakLateralFriction;
    Mechanical.Traction.RelaxationLengthLong = Sheet.RelaxationLengthLong;
    Mechanical.Traction.RelaxationLengthLat = Sheet.RelaxationLengthLat;

    // Mechanical - Rolling Resistance
    Mechanical.RollingResistance.Constant = Sheet.RollingResistanceConstant;
    Mechanical.RollingResistance.VelocityFactor = Sheet.RollingResistanceVelocityFactor;

    // Thermal
    Thermal.TreadThermalMass_J_K = Sheet.TreadThermalMass_J_K;
    Thermal.CarcassThermalMass_J_K = Sheet.CarcassThermalMass_J_K;
    Thermal.TreadCarcassConduction_W_K = Sheet.TreadCarcassConduction_W_K;
    Thermal.ConvectionCoeff_W_m2K = Sheet.ConvectionCoeff_W_m2K;
    Thermal.OptimalTemp_K = Sheet.OptimalTemp_K;
    Thermal.OverheatTemp_K = Sheet.OverheatTemp_K;
    Thermal.GripTempSensitivity = Sheet.GripTempSensitivity;

    // Structural
    Structural.NominalPressure_Pa = Sheet.NominalPressure_Pa;
    Structural.MinPressure_Pa = Sheet.MinPressure_Pa;
    Structural.MaxPressure_Pa = Sheet.MaxPressure_Pa;
    Structural.TensileSidewallStrength_Pa = Sheet.TensileSidewallStrength_Pa;
    Structural.TensileTreadStrength_Pa = Sheet.TensileTreadStrength_Pa;
    Structural.PressureStiffnessGain = Sheet.PressureStiffnessGain;
    Structural.SidewallDampingCoeff_NsPerM = Sheet.SidewallDampingCoeff_NsPerM;
    Structural.SidewallElasticity_N_per_m = Sheet.SidewallElasticity_N_per_m;
    Structural.BeadLockPaddingCm = Sheet.BeadLockPaddingCm;

    // Wear
    Wear.InitialTreadDepth_mm = Sheet.InitialTreadDepth_mm;
    Wear.BaseWearRate_mm_MJ = Sheet.BaseWearRate_mm_MJ;
    Wear.ThermalWearMultiplier = Sheet.ThermalWearMultiplier;
    Wear.LoadWearExponent = Sheet.LoadWearExponent;
    Wear.GripLossPerMM = Sheet.GripLossPerMM;

    // Aquaplaning
    Aquaplaning.HydroplaningCoeff_NASA = Sheet.HydroplaningCoeff_NASA;
    Aquaplaning.DrainageEfficiency = Sheet.DrainageEfficiency;
    Aquaplaning.WetFrictionBase = Sheet.WetFrictionBase;

    // Pacejka - Globals
    Pacejka.Fz0 = Sheet.Fz0;
    Pacejka.Lx = Sheet.Lx;
    Pacejka.Ly = Sheet.Ly;
    Pacejka.Ltr = Sheet.Ltr;
    Pacejka.RelaxationLengthLong = Sheet.PacejkaRelaxationLengthLong;
    Pacejka.RelaxationLengthLat = Sheet.PacejkaRelaxationLengthLat;
    Pacejka.RelaxationLoadExponentLong = Sheet.RelaxationLoadExponentLong;
    Pacejka.RelaxationLoadExponentLat = Sheet.RelaxationLoadExponentLat;

    // Pacejka - Longitudinal
    Pacejka.Longitudinal.pBx1 = Sheet.pBx1;
    Pacejka.Longitudinal.pBx2 = Sheet.pBx2;
    Pacejka.Longitudinal.pBx3 = Sheet.pBx3;
    Pacejka.Longitudinal.pCx1 = Sheet.pCx1;
    Pacejka.Longitudinal.pDx1 = Sheet.pDx1;
    Pacejka.Longitudinal.pDx2 = Sheet.pDx2;
    Pacejka.Longitudinal.pDx3 = Sheet.pDx3;
    Pacejka.Longitudinal.pEx1 = Sheet.pEx1;
    Pacejka.Longitudinal.pEx2 = Sheet.pEx2;
    Pacejka.Longitudinal.pEx3 = Sheet.pEx3;
    Pacejka.Longitudinal.pEx4 = Sheet.pEx4;
    Pacejka.Longitudinal.pHx1 = Sheet.pHx1;
    Pacejka.Longitudinal.pHx2 = Sheet.pHx2;
    Pacejka.Longitudinal.pVx1 = Sheet.pVx1;
    Pacejka.Longitudinal.pVx2 = Sheet.pVx2;

    // Pacejka - Lateral
    Pacejka.Lateral.pBy1 = Sheet.pBy1;
    Pacejka.Lateral.pBy2 = Sheet.pBy2;
    Pacejka.Lateral.pBy3 = Sheet.pBy3;
    Pacejka.Lateral.pCy1 = Sheet.pCy1;
    Pacejka.Lateral.pDy1 = Sheet.pDy1;
    Pacejka.Lateral.pDy2 = Sheet.pDy2;
    Pacejka.Lateral.pDy3 = Sheet.pDy3;
    Pacejka.Lateral.pEy1 = Sheet.pEy1;
    Pacejka.Lateral.pEy2 = Sheet.pEy2;
    Pacejka.Lateral.pEy3 = Sheet.pEy3;
    Pacejka.Lateral.pHy1 = Sheet.pHy1;
    Pacejka.Lateral.pHy2 = Sheet.pHy2;
    Pacejka.Lateral.pVy1 = Sheet.pVy1;
    Pacejka.Lateral.pVy2 = Sheet.pVy2;
    Pacejka.Lateral.pVy3 = Sheet.pVy3;
    Pacejka.Lateral.pVy4 = Sheet.pVy4;

    // Pacejka - Camber
    Pacejka.Camber.pCamber1 = Sheet.pCamber1;
    Pacejka.Camber.pCamber2 = Sheet.pCamber2;

    // Pacejka - Aligning
    Pacejka.Aligning.qDz1 = Sheet.qDz1;
    Pacejka.Aligning.qDz2 = Sheet.qDz2;
    Pacejka.Aligning.qDz3 = Sheet.qDz3;
    Pacejka.Aligning.qBz1 = Sheet.qBz1;
    Pacejka.Aligning.qBz2 = Sheet.qBz2;
    Pacejka.Aligning.qBz3 = Sheet.qBz3;
    Pacejka.Aligning.qBz5 = Sheet.qBz5;
    Pacejka.Aligning.qCz1 = Sheet.qCz1;
    Pacejka.Aligning.qEz1 = Sheet.qEz1;
    Pacejka.Aligning.qEz2 = Sheet.qEz2;
    Pacejka.Aligning.qEz3 = Sheet.qEz3;
    Pacejka.Aligning.qEz4 = Sheet.qEz4;
    Pacejka.Aligning.qEz5 = Sheet.qEz5;

    // Pacejka - Combined Slip Longitudinal
    Pacejka.CombinedLongitudinal.rBx1 = Sheet.rBx1;
    Pacejka.CombinedLongitudinal.rBx2 = Sheet.rBx2;
    Pacejka.CombinedLongitudinal.rBx3 = Sheet.rBx3;
    Pacejka.CombinedLongitudinal.rCx1 = Sheet.rCx1;
    Pacejka.CombinedLongitudinal.rEx1 = Sheet.rEx1;
    Pacejka.CombinedLongitudinal.rEx2 = Sheet.rEx2;
    Pacejka.CombinedLongitudinal.rHx1 = Sheet.rHx1;

    // Pacejka - Combined Slip Lateral
    Pacejka.CombinedLateral.rBy1 = Sheet.rBy1;
    Pacejka.CombinedLateral.rBy2 = Sheet.rBy2;
    Pacejka.CombinedLateral.rBy3 = Sheet.rBy3;
    Pacejka.CombinedLateral.rBy4 = Sheet.rBy4;
    Pacejka.CombinedLateral.rCy1 = Sheet.rCy1;
    Pacejka.CombinedLateral.rEy1 = Sheet.rEy1;
    Pacejka.CombinedLateral.rEy2 = Sheet.rEy2;
    Pacejka.CombinedLateral.rHy1 = Sheet.rHy1;
    Pacejka.CombinedLateral.rHy2 = Sheet.rHy2;
    Pacejka.CombinedLateral.rVy1 = Sheet.rVy1;
    Pacejka.CombinedLateral.rVy2 = Sheet.rVy2;
    Pacejka.CombinedLateral.rVy3 = Sheet.rVy3;
    Pacejka.CombinedLateral.rVy4 = Sheet.rVy4;
    Pacejka.CombinedLateral.rVy5 = Sheet.rVy5;
    Pacejka.CombinedLateral.rVy6 = Sheet.rVy6;

    // Initialize precomputed cache after all values are set
    InitializeCache();

    return *this;
};

/**
====================================================================================================================================================
                                                      PACEJKA PRECOMPUTED CACHE INITIALIZATION
====================================================================================================================================================*/

/** Initialize precomputed Pacejka cache - call once during tire initialization or when parameters change */
inline void FPacejkaPrecomputedCache::Initialize(const FPacejkaModel& Pacejka)
{
    // Global - 🔥 OPTIMIZED: Cache both Fz0 and InvFz0 to avoid runtime division
    Fz0 = FMath::Max(Pacejka.Fz0, 0.001f);
    InvFz0 = 1.0f / Fz0;
    Ly = Pacejka.Ly;

    // Load sensitivity mapping
    MuLoadSensitivityLat = Pacejka.MuLoadSensitivityLat;
    MuLoadSensitivityLong = Pacejka.MuLoadSensitivityLong;
    MuLoadMinFactor = Pacejka.MuLoadMinFactor;

    // Longitudinal precomputed (coefficient * scaling)
    Cx_scaled = Pacejka.Longitudinal.pCx1 * Pacejka.Lx;
    pDx1_Lx = Pacejka.Longitudinal.pDx1 * Pacejka.Lx;
    pDx2_Lx = Pacejka.Longitudinal.pDx2 * Pacejka.Lx;
    pDx3 = Pacejka.Longitudinal.pDx3;
    pBx1_Lx = Pacejka.Longitudinal.pBx1 * Pacejka.Lx;
    pBx2_Lx = Pacejka.Longitudinal.pBx2 * Pacejka.Lx;
    pBx3 = Pacejka.Longitudinal.pBx3;
    pEx1_Lx = Pacejka.Longitudinal.pEx1 * Pacejka.Lx;
    pEx2_Lx = Pacejka.Longitudinal.pEx2 * Pacejka.Lx;
    pEx3_Lx = Pacejka.Longitudinal.pEx3 * Pacejka.Lx;
    pEx4 = Pacejka.Longitudinal.pEx4;
    pHx1_Lx = Pacejka.Longitudinal.pHx1 * Pacejka.Lx;
    pHx2_Lx = Pacejka.Longitudinal.pHx2 * Pacejka.Lx;
    pVx1_Lx = Pacejka.Longitudinal.pVx1 * Pacejka.Lx;
    pVx2_Lx = Pacejka.Longitudinal.pVx2 * Pacejka.Lx;

    // Lateral precomputed (coefficient * scaling)
    Cy_scaled = Pacejka.Lateral.pCy1 * Pacejka.Ly;
    pDy1_Ly = Pacejka.Lateral.pDy1 * Pacejka.Ly;
    pDy2_Ly = Pacejka.Lateral.pDy2 * Pacejka.Ly;
    pDy3 = Pacejka.Lateral.pDy3;
    pBy1_Ly = Pacejka.Lateral.pBy1 * Pacejka.Ly;
    pBy2_Ly = Pacejka.Lateral.pBy2 * Pacejka.Ly;
    pBy3 = Pacejka.Lateral.pBy3;
    pEy1_Ly = Pacejka.Lateral.pEy1 * Pacejka.Ly;
    pEy2_Ly = Pacejka.Lateral.pEy2 * Pacejka.Ly;
    pEy3 = Pacejka.Lateral.pEy3;
    pHy1_Ly = Pacejka.Lateral.pHy1 * Pacejka.Ly;
    pHy2_Ly = Pacejka.Lateral.pHy2 * Pacejka.Ly;
    pVy1_Ly = Pacejka.Lateral.pVy1 * Pacejka.Ly;
    pVy2_Ly = Pacejka.Lateral.pVy2 * Pacejka.Ly;
    pVy3 = Pacejka.Lateral.pVy3;
    pVy4 = Pacejka.Lateral.pVy4;

    // Camber precomputed
    pCamber1_Ly = Pacejka.Camber.pCamber1 * Pacejka.Ly;
    pCamber2_Ly = Pacejka.Camber.pCamber2 * Pacejka.Ly;

    // Combined slip - longitudinal
    rHx1 = Pacejka.CombinedLongitudinal.rHx1;
    rBx1 = Pacejka.CombinedLongitudinal.rBx1;
    rBx2 = Pacejka.CombinedLongitudinal.rBx2;
    rBx3 = Pacejka.CombinedLongitudinal.rBx3;
    rCx1 = Pacejka.CombinedLongitudinal.rCx1;
    rEx1 = Pacejka.CombinedLongitudinal.rEx1;
    rEx2 = Pacejka.CombinedLongitudinal.rEx2;

    // Combined slip - lateral
    rHy1 = Pacejka.CombinedLateral.rHy1;
    rHy2 = Pacejka.CombinedLateral.rHy2;
    rBy1 = Pacejka.CombinedLateral.rBy1;
    rBy2 = Pacejka.CombinedLateral.rBy2;
    rBy3 = Pacejka.CombinedLateral.rBy3;
    rBy4 = Pacejka.CombinedLateral.rBy4;
    rCy1 = Pacejka.CombinedLateral.rCy1;
    rEy1 = Pacejka.CombinedLateral.rEy1;
    rEy2 = Pacejka.CombinedLateral.rEy2;
    rVy1 = Pacejka.CombinedLateral.rVy1;
    rVy2 = Pacejka.CombinedLateral.rVy2;
    rVy3 = Pacejka.CombinedLateral.rVy3;
    rVy4 = Pacejka.CombinedLateral.rVy4;
    rVy5 = Pacejka.CombinedLateral.rVy5;
    rVy6 = Pacejka.CombinedLateral.rVy6;

    // Aligning precomputed
    qDz1_Ltr = Pacejka.Aligning.qDz1 * Pacejka.Ltr;
    qDz2_Ltr = Pacejka.Aligning.qDz2 * Pacejka.Ltr;
    qDz3 = Pacejka.Aligning.qDz3;
    qBz1 = Pacejka.Aligning.qBz1;
    qBz2 = Pacejka.Aligning.qBz2;
    qBz3 = Pacejka.Aligning.qBz3;
    qBz5 = Pacejka.Aligning.qBz5;
    qCz1 = Pacejka.Aligning.qCz1;
    qEz1 = Pacejka.Aligning.qEz1;
    qEz2 = Pacejka.Aligning.qEz2;
    qEz3 = Pacejka.Aligning.qEz3;
    qEz4 = Pacejka.Aligning.qEz4;
    qEz5 = Pacejka.Aligning.qEz5;

    bInitialized = true;
} // End Initialize()