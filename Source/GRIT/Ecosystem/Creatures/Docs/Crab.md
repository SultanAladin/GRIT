# Crab Design

## Role

The crab is a lateral-pressure creature with magnetic pincers.

Gameplay fantasy:

- side-walking hunter
- armored front profile
- magnetic pull from oversized claws
- drags vehicles toward environmental hazards or into a clamp

## Core Identity

Unlike the scorpion, the crab should rarely charge straight forward.

Its personality is:

- lateral movement first
- frontal threat display second
- pincer pull before direct damage

## High-Level State Loop

```text
Idle -> Track -> SideOrbit -> PincerAim -> MagneticPull -> Clamp
                         |                    |
                         v                    v
                      Retreat <- Cooldown <- BreakContact
```

## Skeleton Layout

Recommended bone names:

```text
root
pelvis
carapace
abdomen_stub

leg_L1_coxa
leg_L1_femur
leg_L1_tibia
leg_L1_tarsus
leg_L1_foot

leg_L2_coxa
leg_L2_femur
leg_L2_tibia
leg_L2_tarsus
leg_L2_foot

leg_L3_coxa
leg_L3_femur
leg_L3_tibia
leg_L3_tarsus
leg_L3_foot

leg_L4_coxa
leg_L4_femur
leg_L4_tibia
leg_L4_tarsus
leg_L4_foot

leg_R1_coxa
leg_R1_femur
leg_R1_tibia
leg_R1_tarsus
leg_R1_foot

leg_R2_coxa
leg_R2_femur
leg_R2_tibia
leg_R2_tarsus
leg_R2_foot

leg_R3_coxa
leg_R3_femur
leg_R3_tibia
leg_R3_tarsus
leg_R3_foot

leg_R4_coxa
leg_R4_femur
leg_R4_tibia
leg_R4_tarsus
leg_R4_foot

claw_L_root
claw_L_upper
claw_L_lower
claw_L_pincer_fixed
claw_L_pincer_move

claw_R_root
claw_R_upper
claw_R_lower
claw_R_pincer_fixed
claw_R_pincer_move
```

Required sockets:

- `socket_pincer_L_magnet`
- `socket_pincer_R_magnet`
- `socket_front_probe`
- `socket_center_mass`

## Locomotion

Crabs move sideways by default.

Movement rules:

- locomotion velocity is mostly lateral in local space
- facing should remain biased toward the target
- body can yaw independently from step direction
- emergency backward scuttle is allowed, but only as a recovery move

Gait idea:

- alternating side-step pattern
- low body roll
- feet place in a shallow arc relative to the carapace

## Magnetic Pincer Attack

The pincers should not behave like a physics explosion.

Use a controlled pull:

1. lock the target side with one or both magnet beams
2. apply capped attraction force toward the pincer midpoint
3. side-walk while maintaining tension
4. convert into clamp or shove

This makes the creature readable and fair.

## ML Scope

Suggested observation set, about `40` dims:

- target relative position
- target side exposure
- distance to wall or hazard
- pincer charge levels
- lateral lane clearance
- current orbit direction
- gait stability

Suggested action set, `5` dims:

- move left or right
- turn intent
- magnet use
- clamp use
- disengage use

Recommended policy:

- `40` inputs
- hidden layers `48 -> 24`
- `5` outputs

## Reward Shape

Reward:

- forcing the target sideways
- keeping target in front arc
- pinning against terrain or hazards
- successful clamp

Penalty:

- chasing head-on like a generic spider
- exposing rear weak point
- wasting magnetic pull at bad angles

## Implementation Notes

Recommended C++ classes:

- `ACrabCreature`
- `UCrabPincerComponent`
- `UCrabAnimInstance`

Special requirement:

the locomotion component should expose both:

- `DesiredFacingYaw`
- `DesiredTravelDirection`

The crab will look wrong if those two values are forced to be identical.

