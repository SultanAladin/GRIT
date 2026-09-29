# Tarantula Design

## Role

The tarantula is a heavy ground ambusher.

Gameplay fantasy:

- slow, tense stalking
- low center of mass
- sudden pounce
- body-on-target pin and bite sequence

## Core Identity

This creature should feel heavier and more deliberate than the scorpion.

It wins with:

- stealthy approach
- burst acceleration
- terrifying close-range commitment

## High-Level State Loop

```text
Hide -> Stalk -> ThreatDisplay -> Pounce -> Pin -> Bite
                    |                            |
                    v                            v
                 Circle <- Recover <- BreakOff <- Escape
```

## Skeleton Layout

Recommended bone names:

```text
root
pelvis
cephalothorax
abdomen

leg_L1_coxa
leg_L1_femur
leg_L1_patella
leg_L1_tibia
leg_L1_metatarsus
leg_L1_tarsus
leg_L1_foot

leg_L2_coxa
leg_L2_femur
leg_L2_patella
leg_L2_tibia
leg_L2_metatarsus
leg_L2_tarsus
leg_L2_foot

leg_L3_coxa
leg_L3_femur
leg_L3_patella
leg_L3_tibia
leg_L3_metatarsus
leg_L3_tarsus
leg_L3_foot

leg_L4_coxa
leg_L4_femur
leg_L4_patella
leg_L4_tibia
leg_L4_metatarsus
leg_L4_tarsus
leg_L4_foot

leg_R1_coxa
leg_R1_femur
leg_R1_patella
leg_R1_tibia
leg_R1_metatarsus
leg_R1_tarsus
leg_R1_foot

leg_R2_coxa
leg_R2_femur
leg_R2_patella
leg_R2_tibia
leg_R2_metatarsus
leg_R2_tarsus
leg_R2_foot

leg_R3_coxa
leg_R3_femur
leg_R3_patella
leg_R3_tibia
leg_R3_metatarsus
leg_R3_tarsus
leg_R3_foot

leg_R4_coxa
leg_R4_femur
leg_R4_patella
leg_R4_tibia
leg_R4_metatarsus
leg_R4_tarsus
leg_R4_foot

pedipalp_L_01
pedipalp_L_02
pedipalp_R_01
pedipalp_R_02

chelicera_L
chelicera_R
spinneret_root
```

Required sockets:

- `socket_bite`
- `socket_body_mount`
- `socket_ground_probe_front`
- `socket_ground_probe_rear`

## Locomotion

Use an alternating tetrapod gait like the scorpion, but slower and lower.

Motion notes:

- more vertical compliance in the body
- less tail or claw silhouette
- more visible abdomen drag and body compression during pounce prep

## Attack Pattern

Preferred sequence:

1. stay in cover or low contrast terrain
2. creep to the flank
3. enter a short threat display if seen
4. launch a pounce
5. pin the vehicle body and bite vulnerable points

The pounce arc should be authored with deterministic launch math, not learned from scratch.

## ML Scope

Suggested observation set, about `44` dims:

- target relative position
- whether the target has line of sight
- local cover score
- pounce readiness
- obstacle rays
- body stability
- current exposure level

Suggested action set, `5` dims:

- creep speed
- turn intent
- threat display commit
- pounce commit
- disengage

Recommended policy:

- `44` inputs
- hidden layers `48 -> 24`
- `5` outputs

## Reward Shape

Reward:

- staying unseen
- reaching ambush range
- successful pounce contact
- maintaining a stable pin

Penalty:

- exposing too early
- missed pounces
- unstable landings

## Implementation Notes

Recommended C++ classes:

- `ATarantulaCreature`
- `UTarantulaPounceComponent`
- `UTarantulaAnimInstance`

The pounce component owns:

- windup timer
- launch vector
- landing prediction
- pin state

