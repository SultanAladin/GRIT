# No Man's Land

## Overview
Cross the explosive minefield to advance to next level. Procedurally generated layouts prevent memorization. Reach the other side alive.

## Difficulty Levels

**ROOKIE** - Sparse mine density, slower activation times, clear visual warnings, wider safe corridors

**VETERAN** - Medium density, standard activation speed, reduced warnings, narrow paths

**ELITE** - High density, instant detonation, minimal indicators, chaotic patterns

**LETHAL** - Maximum chaos, overlapping hazard zones, aggressive behavioral mines, environmental interference

## Core Systems

**Random Mine Placement** - Procedural algorithm generates unique field layout each run. Grid-based spawn system ensures playable paths exist but constantly shifts between attempts.

**Random Mine Spawning** - Dynamic spawn system activates additional mines during crossing based on player position, speed, time elapsed, and damage state. Keeps pressure constant throughout run.

## Mine Types

### Static Mines
- **Pressure Plates** - Detonate when hit above speed threshold
- **Magnetic Mines** - Pull vehicle toward them within range
- **EMP Pulses** - Disable vehicle systems temporarily
- **Decoy Mines** - Mix of real and fake, indistinguishable until close
- **Timer Mines** - Countdown detonation forcing time pressure
- **Ghost Mines** - Invisible until within 10 meters
- **Delayed Fuse** - Explode 3 seconds after passing (hits trailing racers)

### Behavioral Mines
- **Stalker Mines** - Roll toward player, accelerate when close
- **Leap Mines** - Jump into path when player passes nearby
- **Cluster Bombs** - Split into 5 smaller mines when triggered
- **Homing Charges** - Launch at player after 2-second delay
- **Mirror Mines** - Detonate on sharp brake/acceleration/turn input
- **Proximity Swarms** - Cloud of small charges tracking movement
- **Chain Detonation** - Trigger sequence of nearby mine explosions

### Dynamic Placement
- **Shifting Grid** - Mines move on hydraulic lifts, constantly repositioning
- **Rotating Sectors** - Field sections rotate 90° every 10 seconds
- **Sinking Platforms** - Safe zones submerge randomly
- **Rising Barriers** - Walls emerge unpredictably blocking paths
- **Sliding Panels** - Floor sections slide laterally with mines
- **Vertical Drops** - Floor sections drop creating pits

### Environmental Hazards
- **Active Turrets** - AI-controlled weapons tracking and firing
- **Collapsing Terrain** - Ground crumbles behind on timer
- **Sandstorm Visibility** - Dust clouds obscure mines randomly
- **Lightning Strikes** - Electrical arcs between mines create barriers
- **Oil Slicks** - Spawn randomly, unpredictable steering
- **Smoke Plumes** - Burning debris blocks vision
- **Acid Rain** - Damages stationary vehicles
- **Wind Bursts** - Push vehicle laterally into mines
- **Gravity Wells** - Zones with altered handling physics
- **Ice Patches** - Reduced traction near clusters

### Player-Influenced Systems
- **Heat Signature Triggers** - Mines activate based on engine temperature
- **Noise Detection** - High RPM/backfire triggers nearby mines
- **Vehicle Mass Sensors** - Heavier vehicles trigger different patterns
- **Speed Gates** - New mines spawn when crossing speed threshold
- **Damage Markers** - More damage = more aggressive mines
- **Combo Multipliers** - Close calls spawn additional hazards
- **Draft Punish** - Following too close triggers path mines

### Tactical Elements
- **Safe Beacons** - Brief 3-second immunity zones (random spawn)
- **Shielded Corridors** - Protected paths collapse after first pass
- **Decoy Flares** - Divert homing mines (limited ammo)
- **EMP Grenades** - Disable mine sector temporarily (cooldown)
- **Boost Pads** - Speed burst surrounded by dense mine rings
- **Repair Stations** - Fix damage but exposed in danger zones
- **Ammo Caches** - Restock weapons in exposed positions

### Pattern Systems
- **Wave Formation** - Mines activate in rolling waves across field
- **Spiral Arms** - Rotating danger zones from center outward
- **Checkerboard Chaos** - Alternating safe/danger squares that flip
- **Ripple Effect** - Explosions create expanding danger radius
- **Quantum Mines** - Exist in two positions until observed
- **Phase Shift** - Mines blink in/out on random timing
- **Gravity Reverse** - Random ceiling driving zones

### AI Opposition
- **Drone Hunters** - Flying bots shooting at player, ignore mines
- **Ground Mechs** - Patrol units activating mines in path
- **Rival Racers** - AI vehicles triggering mines affecting everyone
- **Sentry Guns** - Stationary turrets with tracking lasers
- **Mine Layers** - Vehicles actively dropping new mines during race