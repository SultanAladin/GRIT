# Creature System Architecture

## Goal

Build believable hostile creatures for `GRIT` with:

- C++-first runtime code
- procedural animation driven by math and traces
- lightweight ML through `NeuralLink`
- low-risk shipping path where every creature still works without training

## Core Principle

Do not use ML to drive raw joints.

Use ML only for:

- target choice
- approach angle
- attack timing
- retreat timing
- ability selection

Keep these deterministic in C++:

- leg placement
- body stabilization
- tail / claw / proboscis IK
- grapple constraints
- hit confirmation
- damage rules
- fail-safes and cooldowns

## Runtime Stack

```text
Perception traces + overlap queries
        |
        v
Tactical brain
State machine + optional NeuralLink policy
        |
        v
Locomotion solver
Desired velocity, turn rate, stance, attack pose
        |
        v
Procedural animation solver
Foot targets, body offsets, appendage arcs, aim constraints
        |
        v
Anim instance / native anim nodes
        |
        v
Skeletal mesh bones and gameplay sockets
```

## Unreal Integration

Use the normal Unreal stack in C++:

- `AProceduralCreatureBase : APawn`
- `USkeletalMeshComponent* CreatureMesh`
- `UCreaturePerceptionComponent`
- `UCreatureLocomotionComponent`
- `UCreatureCombatComponent`
- `UNeuralLinkComponent`
- `UCreatureAnimInstance`

Suggested file ownership:

```text
Core/
  AProceduralCreatureBase
  FCreatureTargetInfo
  FCreatureGaitPhase
  FCreatureLegChain
AI/
  UCreatureStateMachineComponent
  UCreatureNeuralBrainComponent
Animation/
  UCreatureAnimInstance
  FProceduralLegSolver
  FProceduralAppendageSolver
Per creature/
  AScorpionCreature
  ACrabCreature
  AMosquitoCreature
  ATarantulaCreature
  AMantisCreature
```

## Shared Update Pipeline

Per tick:

1. Sense nearby vehicles, terrain, ledges, and attack anchors.
2. Produce a desired mode: roam, stalk, orbit, attack, recover, disengage.
3. Convert mode into movement goals and attack goals.
4. Solve gait phases and footstep targets from traces.
5. Solve body pose from support polygon and terrain normals.
6. Solve appendages such as tails, claws, pincers, wings, and proboscis.
7. Push the final pose variables into a native `UAnimInstance`.

## Procedural Animation Strategy

Use a hybrid approach:

- native C++ locomotion solver computes footfalls and body offsets
- native anim instance exposes variables to the Anim Graph
- optional custom anim node handles repeated leg-chain IK in C++
- Blueprints are only for asset hookup and tuning curves

This avoids a Blueprint-heavy procedural animation workflow while still using Unreal's reliable animation pipeline.

## Shared Leg Solver

Each grounded creature should use the same leg-chain model:

- hip/root bone
- upper segment
- lower segment
- foot segment
- end socket

Per leg data:

- default hip transform in body space
- step radius
- lift height
- phase offset
- planted flag
- last planted world point
- desired world point

Step rule:

- if foot error exceeds threshold or support polygon becomes unstable, schedule a step
- only step a legal leg group for the active gait
- place the foot using a forward-predicted target plus terrain trace

## Shared Appendage Solver

Use an appendage solver for:

- scorpion tail
- crab magnetic pincers
- mosquito proboscis
- mantis strike arms
- tarantula pedipalps

The solver should support:

- spline-like segment chains
- aim targets
- curl / open alpha
- tension alpha
- hit-latched end effectors

## NeuralLink Strategy

Your existing `NeuralLink` layer already supports:

- `Creature` entity type
- up to `128` observation values
- up to `16` action values

That is more than enough if the network is only choosing tactics.

Recommended baseline per creature:

- observations: `32` to `64`
- actions: `4` to `8`
- network: `2` hidden layers
- hidden sizes: `64 -> 32` for larger creatures, `48 -> 24` or `32 -> 16` for simpler ones

Do not use giant policies for this project. A 32-layer network is not justified here.

## Training Plan

Use staged training:

1. Scripted baseline
2. Imitation from scripted behavior
3. RL fine-tune for timing and target choice
4. Curriculum with one creature and one vehicle class first

Keep each curriculum narrow:

- flat arena first
- one target first
- disable rare abilities first
- add terrain noise later

## What To Train Vs What To Author

Author in C++:

- walk cycles
- gait phase tables
- IK rules
- grapple constraints
- attack hit windows
- cooldowns
- collision responses

Train with ML:

- when to flank
- when to commit
- whether to grapple or crush
- when to retreat
- which side of a vehicle is safest to approach

## Shared Interfaces

Recommended gameplay interfaces:

- `ICreatureTargetableInterface`
- `IMagneticAttachTargetInterface`
- `IFuelDrainTargetInterface`
- `ICreatureDamageableInterface`

This keeps creature code decoupled from the current vehicle implementation.

## Common Sockets

Every creature skeleton should provide gameplay sockets:

- `socket_mouth`
- `socket_center_mass`
- `socket_ground_probe`
- `socket_attack_tip`
- `socket_vfx_primary`

Creature-specific sockets are listed in each document.

## Shipping Strategy

Each creature should be shippable in three levels:

1. FSM only
2. FSM plus procedural animation
3. FSM plus procedural animation plus NeuralLink policy

If training slips, the creature should still be fun and complete.

