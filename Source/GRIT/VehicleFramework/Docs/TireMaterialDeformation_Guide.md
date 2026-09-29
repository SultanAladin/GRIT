# Tire Material Deformation Guide

This is the phase-1 tire deformation path for `GRIT`.

It does not need a custom render pipeline or a standalone `.usf` shader file.

Use:

- a normal Unreal tire material
- `World Position Offset`
- one `Custom` node inside the material for the deformation math
- the runtime `Tire_*` scalar parameters pushed from `AVehicleSolver`

## Why This Path

- works on the existing static-mesh tire proxies
- keeps the rim/bead zone rigid
- lets the flat patch stay at the ground contact instead of rotating with the mesh
- avoids `RHI` / `RenderCore` / compute shader complexity

## Parameter Names Expected By C++

`VehicleSolver` now writes these scalar parameters onto each tire material instance:

- `Tire_Deform_Enabled`
- `Tire_OuterRadiusCm`
- `Tire_InnerRadiusCm`
- `Tire_HalfWidthCm`
- `Tire_BeadLockPaddingCm`
- `Tire_PressureRatio`
- `Tire_LoadRatio`
- `Tire_ContactAlpha`
- `Tire_WearAlpha`
- `Tire_FlattenAmountCm`
- `Tire_BulgeAmountCm`
- `Tire_PatchHalfAngleRad`
- `Tire_RotationAngleRad`
- `Tire_SuspensionDispCm`

## Material Setup

Assumptions:

- tire mesh is centered on its own origin
- wheel spins around local `Y`
- tire cross-section lives in local `X/Z`
- local units are centimeters

Recommended graph:

1. `Absolute World Position`
2. transform world position to local space
3. feed local position and the scalar params into a `Custom` node
4. transform the resulting local offset back to world space
5. wire that into `World Position Offset`

## Custom Node Inputs

Suggested inputs for the `Custom` node:

- `LocalPos` as `float3`
- `OuterRadiusCm`
- `InnerRadiusCm`
- `HalfWidthCm`
- `BeadLockPaddingCm`
- `PressureRatio`
- `LoadRatio`
- `ContactAlpha`
- `WearAlpha`
- `FlattenAmountCm`
- `BulgeAmountCm`
- `PatchHalfAngleRad`
- `RotationAngleRad`
- `DeformEnabled`

Output type:

- `CMOT Float3`

## Custom Node Code

```hlsl
float3 P = LocalPos;

if (DeformEnabled < 0.5)
{
    return float3(0, 0, 0);
}

float Radius = length(float2(P.x, P.z));
float SafeOuter = max(OuterRadiusCm, 0.001);
float SafeHalfWidth = max(HalfWidthCm, 0.001);
float RigidRadius = InnerRadiusCm + BeadLockPaddingCm;

// Lock the bead/rim-adjacent zone so the inner edge stays intact.
float RadialMask = saturate((Radius - RigidRadius) / max(SafeOuter - RigidRadius, 0.001));
RadialMask = smoothstep(0.0, 0.22, RadialMask);

// Reduce deformation toward the shoulders so the silhouette stays cleaner.
float WidthAlpha = saturate(1.0 - abs(P.y) / SafeHalfWidth);
WidthAlpha = WidthAlpha * WidthAlpha;

// Keep the flat patch at the ground contact by offsetting with wheel rotation.
float Angle = atan2(P.z, P.x);
float Wrapped = atan2(sin(Angle + RotationAngleRad + 1.57079632679), cos(Angle + RotationAngleRad + 1.57079632679));
float PatchMask = saturate(1.0 - abs(Wrapped) / max(PatchHalfAngleRad, 0.001));
PatchMask = PatchMask * PatchMask;

float ContactMask = ContactAlpha * PatchMask * RadialMask;
float FlattenCm = FlattenAmountCm * ContactMask;

// Sidewall bulge fades out near the tread crown and stays off near the bead.
float SidewallAlpha = saturate(1.0 - abs((Radius - RigidRadius) / max(SafeOuter - RigidRadius, 0.001) - 0.45) * 2.2);
float BulgeCm = BulgeAmountCm * SidewallAlpha * WidthAlpha * RadialMask;

float3 Offset = float3(0, 0, 0);

// Flatten bottom contact patch upward in local Z.
Offset.z += FlattenCm;

// Expand sidewall outward in X/Z radial direction.
float2 RadialDir = (Radius > 0.001) ? (float2(P.x, P.z) / Radius) : float2(0.0, 1.0);
Offset.xz += RadialDir * BulgeCm;

return Offset;
```

## Important Note About The Rigid Inner Edge

The part near the rim stays intact because the deformation is masked out until the radius exceeds:

`InnerRadiusCm + BeadLockPaddingCm`

That is the main control for the constraint you asked for.

If the tire still bends too close to the rim:

- increase `Tire_BeadLockPaddingCm`

If the tire looks too stiff:

- reduce `Tire_BeadLockPaddingCm`
- or reduce the `smoothstep` threshold in the custom node

## Expected Result

- low load / high pressure: very subtle contact patch
- high load / low pressure: wider flat patch and larger sidewall bulge
- rotating wheel: the flat patch stays at the ground instead of spinning around with the tire
- rim edge: remains rigid
