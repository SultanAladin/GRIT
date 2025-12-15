#pragma once

#include "CoreMinimal.h"
#include "SuspensionSpecifications.generated.h"

//------------------------------------------------------------------------------
//                                SUSPENSION CURVE TYPES
//------------------------------------------------------------------------------

/** Progressive spring curve type */
UENUM(BlueprintType)
enum class ESuspensionCurveType : uint8
{
    Linear      UMETA(DisplayName = "Linear"),           // Constant rate
    Progressive UMETA(DisplayName = "Progressive"),      // Rising rate
    Digressive  UMETA(DisplayName = "Digressive"),       // Falling rate
    DualRate    UMETA(DisplayName = "Dual Rate")         // Two-stage linear
};

//------------------------------------------------------------------------------
//                    PROGRESSIVE SPRING RATE - CUBIC POLYNOMIAL MODEL
//------------------------------------------------------------------------------

/** Progressive spring force coefficients using cubic polynomial model */
USTRUCT(BlueprintType)
struct FProgressiveSpringCoefficients
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progressive Coefficients")
    float k1 = 0.0f;                                 // [N/m] - Linear stiffness coefficient

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progressive Coefficients")
    float k2 = 0.0f;                                 // [N/m²] - Quadratic hardening coefficient

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progressive Coefficients")
    float k3 = 0.0f;                                 // [N/m³] - Cubic hardening coefficient
    
    /** Check if progressive characteristics are enabled */
    FORCEINLINE bool IsProgressive() const { return FMath::Abs(k2) > UE_SMALL_NUMBER || FMath::Abs(k3) > UE_SMALL_NUMBER; }
};

//------------------------------------------------------------------------------
//                                SUSPENSION SPECIFICATIONS
//------------------------------------------------------------------------------

/** Suspension specification data for tuning, geometry, and travel limits */
USTRUCT(BlueprintType)
struct FSuspensionSpecifications 
{
    GENERATED_BODY()

    //------------------------------------------------------------------------------
    // Core Identifier
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension") 
    FName SuspensionID;

    //------------------------------------------------------------------------------
    // Suspension Tuning
    //------------------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Suspension|Tuning") 
    float SpringRate = 250000.0f;                        // [N/m] - Base linear spring rate (calculated from NaturalFrequency)

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Suspension|Tuning") 
    float DampingRate = 3000.0f;                         // [N·s/m] - Damping coefficient (calculated from DampingRatio)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Tuning") 
    float MaxTravel = 0.35f;                             // [m] - Maximum suspension travel

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Tuning") 
    float Preload = 5000.0f;                             // [N] - Spring preload force

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Tuning", meta = (ClampMin = 0.0)) 
    float StaticPreload = 5.0f;                          // [cm] - Static preload compression

    //------------------------------------------------------------------------------
    // Suspension Travel
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Travel", meta = (ClampMin = 0.0)) 
    float MinRaise = 2.0f;                               // [cm] - Maximum compression (bump)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Travel", meta = (ClampMin = 0.0)) 
    float MaxDrop = 8.0f;                                // [cm] - Maximum extension (droop)

    //------------------------------------------------------------------------------
    // Suspension Geometry (setup)
    //------------------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Suspension|Geometry") 
    float RestLength = 0.0f;                             // [cm] - Equilibrium spring length

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Suspension|Geometry") 
    float SuspensionMountLength = 35.0f;                 // [cm] - Physical length of suspension mount hardware

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Suspension|Geometry") 
    FVector LocalSpringOrigin = FVector::ZeroVector;     // [cm] - Local-space spring attachment point

    //------------------------------------------------------------------------------
    // Suspension Axis (Local Space - for telescopic/angled suspensions)
    //------------------------------------------------------------------------------
    
    /** 
     * Local-space suspension travel axis (normalized direction).
     * Default (0,0,-1) = vertical telescopic (standard car suspension).
     * For angled forks (motorcycles): e.g., (-0.42, 0, -0.91) for 25° rake.
     * 
     * MIRRORING: This is stored as the LEFT wheel axis. Right wheels automatically
     * mirror the Y component at runtime using WheelCode.
     * 
     * USAGE: Transformed to world space each frame via HullXfm.TransformVectorNoScale()
     * to get the actual suspension travel direction that accounts for vehicle tilt.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Geometry")
    FVector LocalSuspensionAxis = FVector(0.0f, 0.0f, -1.0f);  // [-] - Local-space suspension axis (down = compression)

    /** 
     * Set suspension axis from rake angle (for motorcycle-style telescopic forks).
     * @param RakeAngleDeg - Angle from vertical in degrees (positive = tilted forward)
     *                       0° = vertical (standard car), 25° = typical sportbike, 30° = cruiser
     */
    FORCEINLINE void SetSuspensionAxisFromRake(float RakeAngleDeg)
    {
        const float RakeRad = FMath::DegreesToRadians(RakeAngleDeg);
        // Rake tilts the fork forward (negative X) from vertical (negative Z)
        LocalSuspensionAxis = FVector(-FMath::Sin(RakeRad), 0.0f, -FMath::Cos(RakeRad));
    }
    
    /** 
     * Set suspension axis from pitch and roll angles (for complex suspension geometries).
     * @param PitchDeg - Angle from vertical in pitch (forward/back tilt)
     * @param RollDeg - Angle from vertical in roll (left/right tilt) - will be mirrored for right wheels
     */
    FORCEINLINE void SetSuspensionAxisFromAngles(float PitchDeg, float RollDeg)
    {
        const float PitchRad = FMath::DegreesToRadians(PitchDeg);
        const float RollRad = FMath::DegreesToRadians(RollDeg);
        // Build axis from pitch (X-Z plane) and roll (Y-Z plane)
        LocalSuspensionAxis = FVector(
            -FMath::Sin(PitchRad),
            -FMath::Sin(RollRad),
            -FMath::Cos(PitchRad) * FMath::Cos(RollRad)
        ).GetSafeNormal();
    }

    //------------------------------------------------------------------------------
    // Progressive Spring Parameters (Cubic Polynomial Model)
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Progressive")
    bool bUseProgressive = true;                        // [-] - Enable progressive spring characteristics

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Progressive")
    ESuspensionCurveType CurveType = ESuspensionCurveType::Digressive;  // [-] - Spring force curve type

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Progressive", meta = (ClampMin = "0.0"))
    float HardeningFactor = 1.5f;                        // [-] - Progressive rate multiplier at full compression

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Suspension|Progressive")
    FProgressiveSpringCoefficients ProgressiveCoeffs;    // [-] - Cubic polynomial coefficients (computed)

    //------------------------------------------------------------------------------
    // Dynamic Parameters
    //------------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Dynamics")
    float NaturalFrequency = 2.0f;                        // [Hz] - Natural frequency

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Dynamics")
    float DampingRatio = 0.6f;                            // [-] - Damping ratio

    //------------------------------------------------------------------------------
    // Constructors
    //------------------------------------------------------------------------------

    FSuspensionSpecifications() = default;

    //------------------------------------------------------------------------------
    // Effective Spring Rate Calculation (Cubic Polynomial)
    //------------------------------------------------------------------------------

    /** Calculate effective stiffness [N/m] from compression [m] using cubic polynomial */
    FORCEINLINE float GetEffectiveSpringRate_NperM(float CompressionCm) const
    {
        // Reason: Progressive disabled - return base rate
        if (!bUseProgressive || !ProgressiveCoeffs.IsProgressive())
        {
            return SpringRate;
        } // End if (linear mode)

        const float x = CompressionCm * 0.01f;                              // [m] - Convert to meters
        const float x2 = x * x;                                             // [m²]
        const float x3 = x2 * x;                                            // [m³]
        
        // Reason: Cubic polynomial model F = k₁x + k₂x² + k₃x³
        // Instantaneous stiffness is dF/dx = k₁ + 2k₂x + 3k₃x²
        const float k_eff = ProgressiveCoeffs.k1 
                          + 2.0f * ProgressiveCoeffs.k2 * x 
                          + 3.0f * ProgressiveCoeffs.k3 * x2;               // [N/m]
        
        return FMath::Max(k_eff, SpringRate * 0.1f);                        // [N/m] - Clamp minimum to 10% base rate
    }
    
    /** Calculate spring force [N] from compression [m] using cubic polynomial */
    FORCEINLINE float GetSpringForce_N(float CompressionCm) const
    {
        // Reason: Progressive disabled - use linear Hooke's law
        if (!bUseProgressive || !ProgressiveCoeffs.IsProgressive())
        {
            return SpringRate * (CompressionCm * 0.01f);                    // [N]
        } // End if (linear mode)

        const float x = CompressionCm * 0.01f;                              // [m]
        const float x2 = x * x;                                             // [m²]
        const float x3 = x2 * x;                                            // [m³]
        
        // Reason: Cubic polynomial model F = k₁x + k₂x² + k₃x³
        const float F = ProgressiveCoeffs.k1 * x 
                      + ProgressiveCoeffs.k2 * x2 
                      + ProgressiveCoeffs.k3 * x3;                          // [N]
        
        return F;
    }

    /** Compute progressive coefficients from tuning parameters */
FORCEINLINE void ComputeProgressiveCoefficients()
{
    // Reason: Skip computation if progressive disabled
    if (!bUseProgressive)
    {
        ProgressiveCoeffs.k1 = SpringRate;
        ProgressiveCoeffs.k2 = 0.0f;
        ProgressiveCoeffs.k3 = 0.0f;
        return;
    } // End if (linear only)

    const float x_max = (MinRaise + MaxDrop) * 0.01f;               // [m] - Full travel range
    const float F0 = SpringRate * (StaticPreload * 0.01f);          // [N] - Force at static preload
    const float F_max = F0 * HardeningFactor;                       // [N] - Force at max compression

    switch (CurveType)
    {
        case ESuspensionCurveType::Progressive:  // Reason: Rising rate curve
        {
            ProgressiveCoeffs.k1 = SpringRate;                      // [N/m] - Linear base
            ProgressiveCoeffs.k2 = 0.0f;                            // [N/m²]
            ProgressiveCoeffs.k3 = (F_max - SpringRate * x_max) / (x_max * x_max * x_max); // [N/m³]
            break;
        }

        case ESuspensionCurveType::Digressive:  // Reason: Falling rate curve
        {
            ProgressiveCoeffs.k1 = SpringRate * HardeningFactor;    // [N/m] - High initial rate
            ProgressiveCoeffs.k2 = -(SpringRate * (HardeningFactor - 1.0f)) / x_max; // [N/m²] - Softening
            ProgressiveCoeffs.k3 = 0.0f;                            // [N/m³]
            break;
        }

        case ESuspensionCurveType::Linear:      // Reason: Constant rate
        default:
        {
            ProgressiveCoeffs.k1 = SpringRate;
            ProgressiveCoeffs.k2 = 0.0f;
            ProgressiveCoeffs.k3 = 0.0f;
            break;
        }
    } // End switch (curve type)
}
};

//==============================================================================
//                    PROGRESSIVE SUSPENSION CALIBRATION
//==============================================================================

// ❌ REMOVED: ComputeProgressiveCoefficients function
// Progressive coefficients are computed automatically in header via inline methods

//------------------------------------------------------------------------------
//                    SUSPENSION STATE VECTOR
//------------------------------------------------------------------------------

// ❌ DELETE THIS ENTIRE STRUCT - it's redundant with FVehicleSolverAxleData_PT
struct FSuspensionStateVector
{
    float CurrentDisplacement = 0.0f;                    // ❌ -> SpringDisplacements[i]
    float CurrentForce = 0.0f;                           // ❌ -> SpringForces[i]
    bool bIsUnloaded = false;                            // ❌ -> !bIsInContact[i]
    float PrevCompression = std::numeric_limits<float>::quiet_NaN(); // ❌ Not needed
    float StaticLoad = 0.0f;                             // ❌ Derive from sprung mass
    float RideHeight = 5.0f;                             // ❌ Derive from SpringDisplacements[i]
    float WheelLoad = 0.0f;                              // ❌ -> WheelLoads[i]
    float LoadTransferDelta = 0.0f;                      // ❌ Derive: WheelLoads[i] - StaticLoad
};

//------------------------------------------------------------------------------
//                    SUSPENSION PROFILE APPLICATION CONFIG
//------------------------------------------------------------------------------

/** Configuration package for applying suspension profiles to wheel groups */
struct FSuspensionProfileConfig
{
    TArray<int32> WheelIndices;                    // [-] - Target wheel indices in Axles_GT
    TArray<FName> AxleSocketNames;                 // [-] - Axle mount socket names
    TArray<FName> SuspensionSocketNames;           // [-] - Suspension mount socket names
    FSuspensionSpecifications Profile;             // [-] - Suspension specification to apply
    
    /** Validation: ensure all arrays match in size */
    bool IsValid() const
    {
        const int32 Count = WheelIndices.Num();
        return Count > 0 
            && AxleSocketNames.Num() == Count 
            && SuspensionSocketNames.Num() == Count;
    }
};

//------------------------------------------------------------------------------
//                    VEHICLE-LEVEL SUSPENSION CONFIG (Setup Only)
//------------------------------------------------------------------------------
/** FVehicleSuspensionConfig - Vehicle-level suspension configuration */
struct FVehicleSuspensionConfig
{
    //------------------------------------------------------------------------------------------------------------------------
    //                                   LOCAL (VEHICLE-BODY-SPACE) WHEEL GEOMETRY
    //------------------------------------------------------------------------------------------------------------------------
    /** Stores local-space socket locations for suspension + axle mounting points */
    struct FLocalWheelGeometry
    {
        FVector LocalSuspensionMount = FVector::ZeroVector;    // [cm] - Relative to vehicle origin/COM
        FVector LocalAxle = FVector::ZeroVector;               // [cm] - Relative to vehicle origin/COM
    };

    // Setup parameters
    float NaturalFrequency = 2.0f;                   // [Hz] - Target system natural frequency
    float DampingRatio = 0.6f;                       // [-] - Damping ratio
    float LocalGroundDistance = 0.0f;                // [cm] - Local ground distance used during setup
    
    // Derived constants (calculated once)
    float OmegaN = 0.0f;                             // [rad/s] - ω = 2π × NaturalFrequency
    float Tau = 0.0f;                                // [-] - τ = 2π
    float GravityCms2 = 980.0f;                      // [cm/s²] - Gravity in cm/s²
    
    // Center of mass (calculated once, then constant)
    FVector LocalCenterOfMass = FVector::ZeroVector; // [cm] - Local-space CoM of hull
    FVector WorldCenterOfMass = FVector::ZeroVector; // [cm] - World-space CoM
    FVector WorldSuspensionNormal = FVector::UpVector; // [-] - World-space suspension normal

    /** Cached per-wheel sprung masses used for PT publishing */
    TArray<float> SprungMasses;                          // [kg] - Matches Axles_GT ordering
    
    /** Cached wheel geometry (game thread) */
    TArray<FLocalWheelGeometry> WheelGeometry_GameThread; // [-] - Local-space suspension/axle sockets

    /** Initialize derived constants */
    void InitializeDerived()
    {
        Tau = 2.0f * PI;
        OmegaN = Tau * NaturalFrequency;
    }
};