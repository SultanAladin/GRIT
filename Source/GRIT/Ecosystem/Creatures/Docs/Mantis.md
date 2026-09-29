# Mantis Design

## Role

The mantis is a precision striker.

Gameplay fantasy:

- eerie head tracking
- measured stalking sway
- explosive raptorial foreleg snap
- pins prey before tearing into weak points

## Core Identity

This creature should not feel like a generic bug.

It should read as:

- alert
- intelligent
- unnervingly patient
- suddenly violent

## High-Level State Loop

```text
Perch -> Observe -> Stalk -> HeadLock -> Strike -> Pin
                    |                       |
                    v                       v
                  Sway <- Reposition <- Recover <- BreakAway
```

## Skeleton Layout

Recommended bone names:

```text
root
pelvis
thorax
abdomen
neck
head_yaw
head_pitch

foreleg_L_coxa
foreleg_L_femur
foreleg_L_tibia
foreleg_L_tarsus
foreleg_L_grip

foreleg_R_coxa
foreleg_R_femur
foreleg_R_tibia
foreleg_R_tarsus
foreleg_R_grip

midleg_L_coxa
midleg_L_femur
midleg_L_tibia
midleg_L_tarsus
midleg_L_foot

midleg_R_coxa
midleg_R_femur
midleg_R_tibia
midleg_R_tarsus
midleg_R_foot

hindleg_L_coxa
hindleg_L_femur
hindleg_L_tibia
hindleg_L_tarsus
hindleg_L_foot

hindleg_R_coxa
hindleg_R_femur
hindleg_R_tibia
hindleg_R_tarsus
hindleg_R_foot

wing_cover_L
wing_cover_R
```

Required sockets:

- `socket_head_focus`
- `socket_strike_L`
- `socket_strike_R`
- `socket_center_mass`

## Locomotion

Use a tripod gait for grounded travel.

Leg groups:

- Group A: `foreleg_L`, `midleg_R`, `hindleg_L`
- Group B: `foreleg_R`, `midleg_L`, `hindleg_R`

Signature motion:

- slight stalking sway
- head tracks target independently from thorax
- forelegs stay partially cocked while targeting

## Attack Pattern

The mantis should feel like a high-precision hit-confirm creature.

Sequence:

1. maintain line of sight
2. lock head onto a weak point
3. inch into strike distance
4. snap both forelegs forward
5. pin or tear depending on armor result

The head tracking must be procedural and continuous. That is the main personality feature.

## ML Scope

Suggested observation set, about `38` dims:

- target relative position
- weak-point visibility score
- strike distance
- strike cooldown
- local obstacle rays
- current head lock quality

Suggested action set, `5` dims:

- creep speed
- turn intent
- head lock hold
- strike commit
- disengage

Recommended policy:

- `38` inputs
- hidden layers `32 -> 16`
- `5` outputs

## Reward Shape

Reward:

- maintaining lock on weak points
- entering strike distance cleanly
- successful pin or part damage

Penalty:

- striking without lock
- losing line of sight
- overextending the forelegs

## Implementation Notes

Recommended C++ classes:

- `AMantisCreature`
- `UMantisStrikeComponent`
- `UMantisAnimInstance`

The anim instance should own:

- head yaw and pitch solve
- foreleg cock alpha
- foreleg strike alpha
- stalking sway alpha

