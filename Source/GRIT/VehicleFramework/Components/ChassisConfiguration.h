#pragma once

#include "CoreMinimal.h"
#include "ChassisConfiguration.generated.h"

//------------------------------------------------------------------------------
// 🔧 Suspension Type Enumeration
//------------------------------------------------------------------------------
/** Suspension architecture type - determines roll center calculation method */
UENUM(BlueprintType)
enum class ESuspensionType : uint8
{
    /** Telescopic (vertical spring/damper) - RC at mount height */
    Telescopic UMETA(DisplayName = "Telescopic (Vertical)"),
    
    /** Double wishbone (A-arms) - RC from instant center geometry */
    DoubleWishbone UMETA(DisplayName = "Double Wishbone"),
    
    /** MacPherson strut - RC from strut axis and lower arm */
    MacPhersonStrut UMETA(DisplayName = "MacPherson Strut"),
    
    /** Multi-link (complex) - RC from virtual instant center */
    MultiLink UMETA(DisplayName = "Multi-Link"),
    
    /** Solid axle (truck/off-road) - RC at axle center */
    SolidAxle UMETA(DisplayName = "Solid Axle"),
    
    /** Manual override - user specifies RC height directly */
    Manual UMETA(DisplayName = "Manual Override")
};

//------------------------------------------------------------------------------
// 🧩 Chassis Configuration
//------------------------------------------------------------------------------
/** @brief FChassisConfiguration - Geometry and mass properties of the vehicle chassis. */
USTRUCT(BlueprintType)
struct FChassisConfiguration
{
    GENERATED_BODY()

    /** Distance between front and rear axle centers [m] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chassis Geometry")
    float WheelBase = 3.0f; // [m]

    /** Distance between left and right wheel centers [m] */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chassis Geometry")
    float TrackWidth = 1.6f; // [m]

    //--------------------------------------------------------------------------
    // Suspension Configuration
    //--------------------------------------------------------------------------
    
    /** Front suspension architecture type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
    ESuspensionType FrontSuspensionType = ESuspensionType::Telescopic;
    
    /** Rear suspension architecture type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
    ESuspensionType RearSuspensionType = ESuspensionType::Telescopic;
    
    //--------------------------------------------------------------------------
    // Roll Center Configuration
    //--------------------------------------------------------------------------
    
    /** Front roll center height [m] - Geometric pivot point for front suspension
     *  
     *  AUTO-CALCULATED from suspension hardpoint sockets (if available)
     *  OR manually specified if SuspensionType = Manual
     *  
     *  Typical ranges by suspension type:
     *  - Telescopic: 0.30-0.50 m (high, at mount point)
     *  - Double wishbone (race): 0.10-0.15 m
     *  - Double wishbone (street): 0.05-0.10 m
     *  - MacPherson strut: 0.00-0.05 m (very low)
     *  - Solid axle: 0.30-0.50 m (very high)
     *  
     *  Effect on handling:
     *  - Lower RC = More elastic transfer (spring/ARB influence) = Progressive
     *  - Higher RC = More geometric transfer (instant) = Sharp response
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Roll Centers", 
        meta = (ClampMin = "0.0", ClampMax = "0.6", UIMin = "0.0", UIMax = "0.4"))
    float RollCenterHeightFront_m = 0.05f; // [m]
    
    /** Rear roll center height [m] - Geometric pivot point for rear suspension
     *  
     *  AUTO-CALCULATED from suspension hardpoint sockets (if available)
     *  OR manually specified if SuspensionType = Manual
     *  
     *  Typical ranges:
     *  - Telescopic: 0.30-0.50 m (high, at mount point)
     *  - Double wishbone (race): 0.15-0.20 m
     *  - Double wishbone (street): 0.10-0.15 m
     *  - MacPherson strut: 0.05-0.10 m
     *  - Solid axle: 0.30-0.50 m (very high)
     *  
     *  Tuning guide:
     *  - Higher rear RC than front = Understeer tendency (stable)
     *  - Lower rear RC than front = Oversteer tendency (agile)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Roll Centers",
        meta = (ClampMin = "0.0", ClampMax = "0.6", UIMin = "0.0", UIMax = "0.4"))
    float RollCenterHeightRear_m = 0.10f; // [m]
    
    /** Geometry correction factor for telescopic suspension [0.1-1.0]
     *  
     *  Telescopic suspension has very high roll centers (at mount point).
     *  This factor scales them down to simulate control arm geometry effects.
     *  
     *  - 1.0 = No correction (use raw mount height)
     *  - 0.3 = Moderate correction (typical for race car feel)
     *  - 0.1 = Strong correction (very low RC, maximum spring influence)
     *  
     *  Only used when SuspensionType = Telescopic
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Roll Centers",
        meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.1", UIMax = "0.5"))
    float TelescopicCorrectionFactor = 0.15f; // [-]


};


