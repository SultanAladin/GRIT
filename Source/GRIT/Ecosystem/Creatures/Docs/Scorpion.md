# Scorpion Design

## Role

The scorpion is the signature desert predator for `Crimson Gorge`.

Gameplay fantasy:

- fast ground pursuit
- natural eight-leg gait
- raised tail with magnetic hook
- claw-assisted kill sequence
- reels vehicles into a crush window

## Why It Should Be First

It exercises almost every system once:

- procedural gait
- tail chain solve
- grapple logic
- target tracking
- hybrid FSM plus ML tactics

If the scorpion works, the other creatures become much easier.

## High-Level State Loop

```text
Idle -> Alert -> Pursue -> Orbit -> TailAim -> GrappleLatch
                                      |              |
                                      v              v
                                   Miss/Reset <- ReelTarget -> ClawCrush
                                                         |
                                                         v
                                                      Recover
```

## Skeleton Layout

Recommended bone names:

```text
root
pelvis
body_front
body_rear

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

tail_01
tail_02
tail_03
tail_04
tail_05
telson_base
hook_tip
```

Required sockets:

- `socket_tail_hook`
- `socket_claw_L_hit`
- `socket_claw_R_hit`
- `socket_body_center`
- `socket_ground_probe_front`
- `socket_ground_probe_rear`

## Locomotion

Use an alternating tetrapod gait.

Leg groups:

- Group A: `L1`, `R2`, `L3`, `R4`
- Group B: `R1`, `L2`, `R3`, `L4`

Phase idea:

```text
Time 0.00 : Group A plant, Group B swing
Time 0.50 : Group B plant, Group A swing
```

Motion notes:

- pelvis stays low and stable
- body yaw leads slightly into turns
- tail counterbalances hard turns
- front claws open wider during threat and attack states

## Tail Grapple Design

The tail is not a free ragdoll. It should be a directed chain solve.

Stages:

1. `TailAim`: solve tail to predicted vehicle anchor point.
2. `GrappleFire`: short-range magnet pulse from `socket_tail_hook`.
3. `GrappleLatch`: attach to a valid metal anchor.
4. `ReelTarget`: apply pull force through controlled constraint logic.
5. `ClawCrush`: close claws and trigger the kill window.

Use deterministic constraints for the reel. Do not train this.

## Combat Behavior

Preferred sequence:

1. Approach from rear quarter or side
2. Orbit until hook angle is good
3. Fire tail hook
4. Pull target off line
5. Step in and crush with claws

Fallback sequence:

- if the hook misses, side-strafe and retry
- if the hook breaks, sprint away for a short recovery
- if the target is too large, sting and disengage instead of full crush

## ML Scope

Train only tactical choice.

Suggested observation set, roughly `56` dims:

- target relative position and velocity
- target forward vector
- target threat score
- current hook cooldown
- current claw cooldown
- terrain slope under body
- local obstacle rays
- distance to valid grapple anchors
- current gait stability
- current body damage

Suggested action set, `6` dims:

- move intent
- strafe intent
- turn intent
- hook commit
- crush commit
- disengage intent

Recommended policy:

- actor-critic
- `56` inputs
- hidden layers `64 -> 32`
- `6` outputs

## Reward Shape

Reward:

- closing to useful attack range
- keeping a viable hook angle
- successful latch
- pulling target into crush range
- landing a full crush

Penalty:

- taking avoidable damage
- colliding with walls
- overcommitting when cooldowns are empty
- falling into unstable footing

## Implementation Notes

Recommended C++ classes:

- `AScorpionCreature`
- `UScorpionTailComponent`
- `UScorpionCombatComponent`
- `UScorpionAnimInstance`

The tail component owns:

- hook targeting
- segment curl
- latch state
- reel tension

The anim instance reads:

- gait phases
- body lean
- tail aim alpha
- claw open alpha
- threat pose alpha

