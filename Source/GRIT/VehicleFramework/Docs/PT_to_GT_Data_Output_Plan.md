# PT to GT Data Output Implementation Plan

## Overview
This document outlines the plan for outputting data from Physics Tick (PT) to Game Tick (GT) in the GRIT vehicle framework. The goal is to create a bridge between the high-frequency physics simulation and the game thread for data visualization, telemetry, and gameplay systems.

## Current Architecture Analysis

### Physics Thread (PT)
- **Location**: `AVehicleSolver` class handles physics simulation
- **Frequency**: High-frequency physics updates (sub-stepping)
- **Data**: Complete vehicle state, forces, timing metrics
- **Current Output**: Debug visualization, internal telemetry logging

### Game Thread (GT)  
- **Location**: `AVehicleController` and game systems
- **Frequency**: Standard UE tick rate (typically 60-120 Hz)
- **Data**: Gameplay-relevant vehicle state
- **Current Input**: Limited vehicle state access

## Implementation Strategy

### Phase 1: Data Structure Design
1. **Create PT-to-GT Data Container**
   - Define `FPhysicsToGameData` struct
   - Include essential vehicle state (position, velocity, forces)
   - Add telemetry timing data from existing `FTelemetryFrame`
   - Include aerodynamic forces and suspension data

2. **Thread-Safe Buffer System**
   - Implement double-buffering or lock-free queue
   - Prevent race conditions between PT and GT
   - Handle data overflow scenarios

### Phase 2: Physics Thread Integration
1. **Data Collection Point**
   - Hook into existing `FTelemetryLogger` system
   - Add PT-to-GT data population in physics solve
   - Leverage existing timing infrastructure (`SCOPE_TIMER`)

2. **Buffer Writing**
   - Write completed physics frames to shared buffer
   - Maintain frame timestamps for GT synchronization
   - Handle sub-stepping data aggregation

### Phase 3: Game Thread Consumption
1. **Data Access Interface**
   - Add `GetPhysicsData()` method to `AVehicleSolver`
   - Implement thread-safe data retrieval
   - Provide data validation and interpolation

2. **Consumer Integration**
   - Update `AVehicleController` to consume physics data
   - Enable UI systems to access real-time telemetry
   - Support debugging and analysis tools

### Phase 4: Performance Optimization
1. **Data Filtering**
   - Selective data output based on consumer needs
   - Configurable update rates for different data types
   - Memory-efficient data packing

2. **Network Considerations**
   - Prepare data structure for future network replication
   - Delta compression for bandwidth efficiency
   - Client-side prediction support

## Technical Implementation Details

### Data Structure Definition
```cpp
struct FPhysicsToGameData
{
    // Core vehicle state
    FVector WorldPosition;
    FVector WorldVelocity;
    FRotator WorldRotation;
    FVector AngularVelocity;
    
    // Forces and dynamics
    FVector TotalForce;
    FVector TotalTorque;
    FAerodynamicForces AeroForces;
    
    // Wheel data (4 wheels)
    TArray<FWheelState> WheelStates;
    
    // Performance metrics
    FTelemetryFrame TimingData;
    
    // Metadata
    double PhysicsTimestamp;
    int32 FrameNumber;
};
```

### Thread Safety Mechanisms
- **Double Buffering**: Two data buffers, one for writing, one for reading
- **Atomic Swaps**: Lock-free buffer switching
- **Version Control**: Frame numbers to detect stale data

### Integration Points
1. **Physics Thread**: End of `SolveVehiclePhysics()` 
2. **Game Thread**: Beginning of `Tick()` in `AVehicleController`
3. **Debug Systems**: Enhanced visualization with real-time data

## Benefits
- **Real-time Telemetry**: Live physics data for UI and debugging
- **Performance Monitoring**: Detailed timing analysis across threads
- **Future Networking**: Foundation for multiplayer vehicle replication
- **Debugging Tools**: Enhanced physics visualization and analysis

## Risks and Mitigations
- **Performance Impact**: Minimize with efficient data structures and selective output
- **Thread Safety**: Use proven lock-free patterns and extensive testing
- **Data Latency**: Handle with interpolation and prediction algorithms

## Next Steps
1. Implement basic data structure and buffer system
2. Add physics thread data collection
3. Create game thread consumption interface
4. Integrate with existing telemetry and debug systems
5. Performance testing and optimization

---

*This plan leverages existing GRIT infrastructure (TelemetryLogger, timing systems) while providing a clean separation between physics and game concerns.*
