#pragma once

#include "CoreMinimal.h"
#include "BrakingSpecifications.generated.h"

//------------------------------------------------------------------------------
//                          BRAKING SPECIFICATIONS
//------------------------------------------------------------------------------
/**
 * Academic Foundation
 * Based on Pacejka's "Tire and Vehicle Dynamics" (3rd Ed.) and SAE J2909 (Vehicle Dynamics Terminology):
 *
 * Core Brake Physics
 *   T_brake = μ(T) × F_clamp × R_eff × η_mech
 *   μ(T): Temperature-dependent friction coefficient
 *   F_clamp: Hydraulic clamping force = P_hydraulic × A_piston
 *   R_eff: Effective braking radius (mean of inner/outer disk radius)
 *   η_mech: Mechanical efficiency (~0.98)
 *
 * Heat Generation & Fade
 *   Q̇ = T_brake × |ω_wheel|           // [W] - Power dissipation
 *   dT/dt = Q̇ / (m_rotor × c_p)        // [K/s] - Rotor temperature rise
 *   μ(T) = μ_cold × (1 - k_fade × (T - T_ref) / T_fade)
 */

/** Disk brake system with thermal fade and pressure dynamics */
USTRUCT(BlueprintType)
struct FBrakingSpecifications
{
    GENERATED_BODY()

    //--------------------------------------------------------------------------
    // Disk Geometry
    //--------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Disk")
    float DiskOuterRadius = 0.16f;                       // [m]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Disk")
    float DiskInnerRadius = 0.09f;                       // [m]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Disk")
    float DiskThickness = 0.028f;                        // [m]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Disk")
    int32 NumPistons = 4;                                // [-]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Disk")
    float PistonDiameter = 0.044f;                       // [m]

    //--------------------------------------------------------------------------
    // Friction Properties
    //--------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Friction")
    float FrictionCoeffCold = 0.42f;                     // [-] - Ambient temp

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Friction")
    float FrictionCoeffHot = 0.38f;                      // [-] - Fade temp

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Friction")
    float FadeStartTemp = 573.15f;                       // [K] - 300°C

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Friction")
    float FadeEndTemp = 873.15f;                         // [K] - 600°C

    //--------------------------------------------------------------------------
    // Hydraulic System
    //--------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Hydraulic")
    float MaxBrakePressure = 12.0e6f;                    // [Pa] - 120 bar

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Hydraulic")
    float MaxHandbrakePressure = 9.5e6f;                // [Pa] - 95 bar (GT-R NISMO spec)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Hydraulic")
    float PressureRiseRate = 50.0e6f;                    // [Pa·s⁻¹]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Hydraulic")
    float PressureFallRate = 80.0e6f;                    // [Pa·s⁻¹]

    //--------------------------------------------------------------------------
    // Thermal Properties
    //--------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Thermal")
    float DiskMass = 6.5f;                               // [kg]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Thermal")
    float SpecificHeatCapacity = 460.0f;                 // [J·kg⁻¹·K⁻¹] - Cast iron

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Thermal")
    float ConvectionCoeff = 85.0f;                       // [W·m⁻²·K⁻¹]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Thermal")
    float DiskSurfaceArea = 0.095f;                      // [m²]

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Braking|Thermal")
    float AmbientTemp = 293.15f;                         // [K] - 20°C

    //--------------------------------------------------------------------------
    // Constructors
    //--------------------------------------------------------------------------

    FBrakingSpecifications() = default;

    /** Calculate brake torque from pressure and temperature */
    FORCEINLINE float CalculateBrakeTorque(float Pressure, float Temperature) const
    {
        // Reason: get temperature-dependent friction coefficient
        const float FrictionCoeff = GetFrictionCoefficient(Temperature); // [-]

        // Reason: calculate total piston area
        const float PistonRadius = PistonDiameter * 0.5f; // [m]
        const float SinglePistonArea = PI * PistonRadius * PistonRadius; // [m²]
        const float TotalPistonArea = SinglePistonArea * NumPistons; // [m²]

        // Reason: calculate clamping force
        const float ClampingForce = Pressure * TotalPistonArea; // [N] = [Pa] × [m²]

        // Reason: effective radius for torque calculation
        const float EffectiveRadius = (DiskOuterRadius + DiskInnerRadius) * 0.5f; // [m]

        // Reason: torque on both sides of disk
        const float BrakeTorque = 2.0f * FrictionCoeff * ClampingForce * EffectiveRadius; // [N·m]

        return BrakeTorque;
    } // End CalculateBrakeTorque

    /** Get friction coefficient at given temperature */
    FORCEINLINE float GetFrictionCoefficient(float Temperature) const
    {
        // Reason: no fade before fade start
        if (Temperature <= FadeStartTemp) return FrictionCoeffCold; // End if (cold)

        // Reason: max fade at or above end temp
        if (Temperature >= FadeEndTemp) return FrictionCoeffHot; // End if (hot)

        // Reason: linear fade interpolation
        const float FadeRatio = (Temperature - FadeStartTemp) / (FadeEndTemp - FadeStartTemp); // [-]
        return FMath::Lerp(FrictionCoeffCold, FrictionCoeffHot, FadeRatio); // [-]
    } // End GetFrictionCoefficient

    /** Calculate heat generation rate from brake torque and angular velocity */
    FORCEINLINE float CalculateHeatGeneration(float BrakeTorque, float AngularVelocity) const
    {
        return BrakeTorque * FMath::Abs(AngularVelocity); // [W] = [N·m] × [rad·s⁻¹]
    } // End CalculateHeatGeneration

    /** Calculate cooling rate from temperature and airspeed */
    FORCEINLINE float CalculateCoolingRate(float Temperature, float Airspeed) const
    {
        // Reason: convection increases with airspeed
        const float AirspeedFactor = 1.0f + 0.5f * FMath::Sqrt(FMath::Max(0.0f, Airspeed / 30.0f)); // [-]
        const float EffectiveConvection = ConvectionCoeff * AirspeedFactor; // [W·m⁻²·K⁻¹]

        // Reason: Newton's law of cooling
        const float DeltaT = Temperature - AmbientTemp; // [K]
        return EffectiveConvection * DiskSurfaceArea * DeltaT; // [W]
    } // End CalculateCoolingRate

}; // End FBrakingSpecifications

//------------------------------------------------------------------------------
//                          BRAKING STATE VECTOR
//------------------------------------------------------------------------------

/** Runtime state for a single brake instance */
struct FBrakingStateVector
{
    float CurrentPressure = 0.0f;                        // [Pa] - Hydraulic pressure
    float CurrentTemperature = 293.15f;                  // [K] - Disk temperature
    float CurrentTorque = 0.0f;                          // [N·m] - Output torque
    float CurrentFrictionCoeff = 0.42f;                  // [-] - Active friction
    float HeatGenerationRate = 0.0f;                     // [W] - Heat production
    float CoolingRate = 0.0f;                            // [W] - Heat dissipation
}; // End FBrakingStateVector
