#include "AnimUtil.h"

float UAnimUtil::ComputeCurve(float Progress, EMotionCurve Curve)
{
    Progress = FMath::Clamp(Progress, 0.0f, 1.0f);

    switch (Curve) // Reason: Apply curve transformation
    {
        case EMotionCurve::Linear:     return Progress;
        case EMotionCurve::QuadIn:     return ApplyQuadIn(Progress);
        case EMotionCurve::QuadOut:    return ApplyQuadOut(Progress);
        case EMotionCurve::QuadInOut:  return ApplyQuadInOut(Progress);
        case EMotionCurve::CubicIn:    return ApplyCubicIn(Progress);
        case EMotionCurve::CubicOut:   return ApplyCubicOut(Progress);
        case EMotionCurve::CubicInOut: return ApplyCubicInOut(Progress);
        case EMotionCurve::ExpIn:      return ApplyExpIn(Progress);
        case EMotionCurve::ExpOut:     return ApplyExpOut(Progress);
        case EMotionCurve::Snap:       return (Progress >= 1.0f) ? 1.0f : 0.0f;
        default:                       return Progress;
    } // End switch (Curve)
}

float UAnimUtil::LerpCurved(float Start, float Target, float Progress, EMotionCurve Curve)
{
    float AdjustedProgress = ComputeCurve(Progress, Curve);
    return FMath::Lerp(Start, Target, AdjustedProgress);
}

FLinearColor UAnimUtil::LerpColorCurved(FLinearColor Start, FLinearColor Target, float Progress, EMotionCurve Curve)
{
    float AdjustedProgress = ComputeCurve(Progress, Curve);
    return FMath::Lerp(Start, Target, AdjustedProgress);
}

FVector2D UAnimUtil::LerpVector2DCurved(FVector2D Start, FVector2D Target, float Progress, EMotionCurve Curve)
{
    float AdjustedProgress = ComputeCurve(Progress, Curve);
    return FMath::Lerp(Start, Target, AdjustedProgress);
}

//------------------------------------------------------------------------------
// curve implementations
//------------------------------------------------------------------------------

float UAnimUtil::ApplyQuadIn(float T)
{
    return T * T; // [t²] - Quadratic acceleration
}

float UAnimUtil::ApplyQuadOut(float T)
{
    return 1.0f - (1.0f - T) * (1.0f - T); // [1-(1-t)²] - Quadratic deceleration
}

float UAnimUtil::ApplyQuadInOut(float T)
{
    if (T < 0.5f) // Reason: First half accelerates
    {
        return 2.0f * T * T; // [2t²]
    } // End if (First half)
    else
    {
        float Temp = 2.0f * T - 2.0f;
        return 1.0f - 0.5f * Temp * Temp; // [1-0.5(2t-2)²]
    }
}

float UAnimUtil::ApplyCubicIn(float T)
{
    return T * T * T; // [t³] - Cubic acceleration
}

float UAnimUtil::ApplyCubicOut(float T)
{
    float Temp = 1.0f - T;
    return 1.0f - Temp * Temp * Temp; // [1-(1-t)³] - Cubic deceleration
}

float UAnimUtil::ApplyCubicInOut(float T)
{
    if (T < 0.5f) // Reason: First half accelerates
    {
        return 4.0f * T * T * T; // [4t³]
    } // End if (First half)
    else
    {
        float Temp = 2.0f * T - 2.0f;
        return 1.0f + 0.5f * Temp * Temp * Temp; // [1+0.5(2t-2)³]
    }
}

float UAnimUtil::ApplyExpIn(float T)
{
    if (T <= 0.0f) // Reason: Avoid pow(2,0) at start
    {
        return 0.0f;
    } // End if (Zero check)
    
    return FMath::Pow(2.0f, 10.0f * (T - 1.0f)); // [2^(10(t-1))] - Exponential acceleration
}

float UAnimUtil::ApplyExpOut(float T)
{
    if (T >= 1.0f) // Reason: Avoid pow(2,0) at end
    {
        return 1.0f;
    } // End if (One check)
    
    return 1.0f - FMath::Pow(2.0f, -10.0f * T); // [1-2^(-10t)] - Exponential deceleration
}
