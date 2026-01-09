#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimUtil.generated.h"

/*====================================================================================================================================
                                                         ANIMATION UTILITIES
======================================================================================================================================*/

/** Motion curve types for animations */
UENUM(BlueprintType)
enum class EMotionCurve : uint8
{
    Linear,      // [Linear] - Constant velocity
    QuadIn,      // [Quadratic] - Accelerating start
    QuadOut,     // [Quadratic] - Decelerating end
    QuadInOut,   // [Quadratic] - Smooth acceleration and deceleration
    CubicIn,     // [Cubic] - Sharp acceleration start
    CubicOut,    // [Cubic] - Sharp deceleration end
    CubicInOut,  // [Cubic] - Dramatic ease in/out
    ExpIn,       // [Exponential] - Very slow start, explosive end
    ExpOut,      // [Exponential] - Explosive start, very slow end
    Snap         // [Step] - Instant completion
};

/** Static animation calculation toolkit */
UCLASS()
class GRIT_API UAnimUtil : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Compute curve-adjusted progress [0,1] → [0,1] */
    UFUNCTION(BlueprintPure, Category = "Animation")
    static float ComputeCurve(float Progress, EMotionCurve Curve);

    /** Interpolate value with curve adjustment */
    UFUNCTION(BlueprintPure, Category = "Animation")
    static float LerpCurved(float Start, float Target, float Progress, EMotionCurve Curve);

    /** Interpolate color with curve adjustment */
    UFUNCTION(BlueprintPure, Category = "Animation")
    static FLinearColor LerpColorCurved(FLinearColor Start, FLinearColor Target, float Progress, EMotionCurve Curve);

    /** Interpolate vector with curve adjustment */
    UFUNCTION(BlueprintPure, Category = "Animation")
    static FVector2D LerpVector2DCurved(FVector2D Start, FVector2D Target, float Progress, EMotionCurve Curve);

private:
    static float ApplyQuadIn(float T);    // [Quadratic] - t²
    static float ApplyQuadOut(float T);   // [Quadratic] - 1-(1-t)²
    static float ApplyQuadInOut(float T); // [Quadratic] - Piecewise quad
    static float ApplyCubicIn(float T);   // [Cubic] - t³
    static float ApplyCubicOut(float T);  // [Cubic] - 1-(1-t)³
    static float ApplyCubicInOut(float T);// [Cubic] - Piecewise cubic
    static float ApplyExpIn(float T);     // [Exponential] - 2^(10(t-1))
    static float ApplyExpOut(float T);    // [Exponential] - 1-2^(-10t)
};
