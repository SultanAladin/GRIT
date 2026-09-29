# Mosquito Design

## Role

The mosquito is an aerial parasite for `Shimmer Swamp`.

Gameplay fantasy:

- irritating fast flight
- smart landing on moving vehicles
- precise fuel-drain attack through a proboscis
- evasive takeoff before being crushed

## Core Identity

This creature lives or dies by motion quality.

Do not fake it with a generic flying pawn.

It needs:

- hover noise
- curved approach paths
- brief perch behavior
- believable proboscis alignment

## High-Level State Loop

```text
Roam -> Detect -> Orbit -> DiveApproach -> Perch -> Probe
                                  |                 |
                                  v                 v
                               Evade <- DrainFuel <- Detach
```

## Skeleton Layout

Recommended bone names:

```text
root
thorax
abdomen
head
proboscis_base
proboscis_mid
proboscis_tip

wing_L_root
wing_L_mid
wing_L_tip

wing_R_root
wing_R_mid
wing_R_tip

leg_L1_root
leg_L1_mid
leg_L1_tip

leg_L2_root
leg_L2_mid
leg_L2_tip

leg_L3_root
leg_L3_mid
leg_L3_tip

leg_R1_root
leg_R1_mid
leg_R1_tip

leg_R2_root
leg_R2_mid
leg_R2_tip

leg_R3_root
leg_R3_mid
leg_R3_tip
```

Required sockets:

- `socket_proboscis_tip`
- `socket_perch_center`
- `socket_wing_vfx_L`
- `socket_wing_vfx_R`

## Flight Model

Do not train the wing beat.

Use authored flight math:

- wing flapping from oscillators and curves
- body tilt from desired acceleration
- orbit path from steering behaviors
- random noise only as a small modulation layer

Recommended flight phases:

- hover
- lateral slip
- dive
- flare
- detach burst

## Perch And Fuel Drain

The mosquito should attach to a moving vehicle with a deterministic perch solver.

Perch sequence:

1. choose a valid landing zone
2. align body normal to local vehicle surface
3. plant legs
4. run short proboscis IK to a drain point
5. start siphon

Use an interface for draining instead of hard-coding vehicle types:

- `IFuelDrainTargetInterface`

Possible target data:

- drain sockets
- current fuel amount
- armor factor
- whether the area is exposed

## ML Scope

Suggested observation set, about `52` dims:

- target position and velocity
- vehicle angular velocity
- safe landing zone score
- line-of-sight state
- obstacle rays
- wing damage
- current drain progress
- current escape path score

Suggested action set, `6` dims:

- thrust forward
- strafe
- ascend or descend
- yaw
- commit to perch
- detach now

Recommended policy:

- `52` inputs
- hidden layers `64 -> 32`
- `6` outputs

## Reward Shape

Reward:

- entering stable orbit
- successful perch
- sustained fuel drain
- clean escape

Penalty:

- colliding with terrain
- landing on invalid surfaces
- staying attached too long under threat

## Implementation Notes

Recommended C++ classes:

- `AMosquitoCreature`
- `UMosquitoFlightComponent`
- `UMosquitoPerchComponent`
- `UMosquitoAnimInstance`

Important split:

- the flight component owns steering, avoidance, and perch transforms
- the anim instance owns wing amplitude, abdomen curl, and leg plant alpha

