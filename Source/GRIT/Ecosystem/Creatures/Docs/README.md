# Creature AI And Procedural Animation Docs

This folder is the C++-first design pass for GRIT creature enemies.

Design rules for this system:

- Use Unreal's normal runtime stack in C++: `APawn`, `UActorComponent`, `UAnimInstance`, custom native anim nodes if needed.
- Do not depend on a Blueprint-only procedural animation framework.
- Keep neural networks small. Let deterministic C++ handle locomotion, IK, hit logic, and safety constraints.
- Train tactics, not bones. The policy should choose intent; the animation solver should make that intent look alive.

Recommended implementation order:

1. `CreatureSystem_Architecture.md`
2. `Scorpion.md`
3. `Crab.md`
4. `Mosquito.md`
5. `Tarantula.md`
6. `Mantis.md`

Recommended folder layout when implementation starts:

```text
Ecosystem/Creatures/
|-- Docs/
|-- Core/
|   |-- ProceduralCreatureBase.h/.cpp
|   |-- CreatureTypes.h
|   |-- CreaturePerceptionComponent.h/.cpp
|   |-- CreatureCombatComponent.h/.cpp
|   `-- CreatureLocomotionComponent.h/.cpp
|-- Animation/
|   |-- CreatureAnimInstance.h/.cpp
|   |-- ProceduralLegSolver.h/.cpp
|   `-- ProceduralAppendageSolver.h/.cpp
|-- AI/
|   |-- CreatureStateMachineComponent.h/.cpp
|   `-- CreatureNeuralBrainComponent.h/.cpp
|-- Scorpion/
|-- Crab/
|-- Mosquito/
|-- Tarantula/
`-- Mantis/
```

Short version:

- Scorpion: strongest showcase creature and best first implementation target.
- Crab: reuse side-body and pincer logic after scorpion lands.
- Mosquito: flight/perch/fuel-drain specialist.
- Tarantula: grounded burst predator.
- Mantis: head tracking and snap-strike specialist.

