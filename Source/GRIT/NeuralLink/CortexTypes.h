#pragma once
// CortexTypes.h — Shared structs used by both CortexServer and Unreal NeuralLink.
// This file is included in both codebases — keep it C++17 compatible with no UE dependencies.

#include <cstdint>

namespace Cortex
{

// Maximum dimensions
static constexpr uint32_t MAX_OBSERVATION_DIM = 128;
static constexpr uint32_t MAX_ACTION_DIM = 16;
static constexpr uint32_t MAX_ENTITY_TYPE_NAME = 32;

// IPC channel name
static constexpr const char* IPC_CHANNEL_NAME = "CortexIPC_GRIT";

// Entity type identifiers
enum class EntityType : uint32_t
{
    Vehicle     = 0,
    Human       = 1,
    Creature    = 2,
    Aircraft    = 3,
    Transport   = 4,
    Custom      = 255
};

// Observation vector (variable length, max MAX_OBSERVATION_DIM)
struct FObservationVector
{
    uint32_t EntityTypeHash;
    uint32_t InstanceId;
    uint16_t Dim;
    uint16_t Padding;
    float Data[MAX_OBSERVATION_DIM];
};

// Action vector (variable length, max MAX_ACTION_DIM)
struct FActionVector
{
    uint32_t EntityTypeHash;
    uint32_t InstanceId;
    uint16_t Dim;
    uint16_t Padding;
    float Data[MAX_ACTION_DIM];
};

// Reward signal
struct FRewardSignal
{
    uint32_t EntityTypeHash;
    uint32_t InstanceId;
    float Reward;
    bool bDone;       // Episode terminal
    bool bTruncated;  // Time limit hit (not failure)
    uint8_t Padding[2];
};

// Training status (Server → Unreal)
struct FTrainingStatus
{
    uint32_t Episode;
    uint32_t TotalSteps;
    float MeanReward;
    float BestReward;
    float PolicyLoss;
    float ValueLoss;
    float Entropy;
    float LearningRate;
    bool bTraining;
    bool bConnected;
    uint8_t Padding[2];
};

// Entity registration config
struct FEntityRegistration
{
    uint32_t EntityTypeHash;
    uint16_t ObservationDim;
    uint16_t ActionDim;
    float ActionMin;
    float ActionMax;
    char TypeName[MAX_ENTITY_TYPE_NAME];
};

// Vehicle-specific observation indices (matches the 47-dim vector in the plan)
namespace VehicleObs
{
    static constexpr uint32_t RAYCAST_START = 0;
    static constexpr uint32_t RAYCAST_COUNT = 36;
    static constexpr uint32_t FORWARD_SPEED = 36;
    static constexpr uint32_t LATERAL_SPEED = 37;
    static constexpr uint32_t HEADING_COS = 38;
    static constexpr uint32_t HEADING_SIN = 39;
    static constexpr uint32_t CURRENT_STEERING = 40;
    static constexpr uint32_t CURRENT_THROTTLE = 41;
    static constexpr uint32_t CURRENT_BRAKE = 42;
    static constexpr uint32_t GEAR_RATIO = 43;
    static constexpr uint32_t AVG_SLIP_RATIO = 44;
    static constexpr uint32_t LATERAL_G = 45;
    static constexpr uint32_t YAW_RATE = 46;
    static constexpr uint32_t TOTAL_DIM = 47;
}

// Creature observation layout (AIPerception-driven, 24-dim baseline)
// Matches: 32-64 input target from CreatureSystem_Architecture.md
namespace CreatureObs
{
    // Self state [0..5]
    static constexpr uint32_t SELF_FORWARD_SPEED  = 0;
    static constexpr uint32_t SELF_LATERAL_SPEED  = 1;
    static constexpr uint32_t SELF_YAW_RATE       = 2;
    static constexpr uint32_t SELF_HEALTH_NORM    = 3;
    static constexpr uint32_t SELF_STAMINA_NORM   = 4;
    static constexpr uint32_t SELF_GROUND_NORMAL_Z = 5;

    // Primary target (sight) [6..13]
    static constexpr uint32_t SIGHT_HAS_TARGET    = 6;   // 0/1
    static constexpr uint32_t SIGHT_DISTANCE_NORM = 7;   // [0,1], dist/maxSightRange
    static constexpr uint32_t SIGHT_DIR_FORWARD   = 8;   // dot(selfForward, toTarget)
    static constexpr uint32_t SIGHT_DIR_RIGHT     = 9;   // dot(selfRight, toTarget)
    static constexpr uint32_t SIGHT_HEIGHT_DELTA  = 10;  // (targetZ - selfZ) / maxRange
    static constexpr uint32_t SIGHT_TARGET_SPEED  = 11;  // normalized target speed
    static constexpr uint32_t SIGHT_TARGET_APPROACH = 12; // closing velocity, normalized
    static constexpr uint32_t SIGHT_AGE_NORM      = 13;  // time since last seen, [0,1]

    // Secondary cue (hearing) [14..18]
    static constexpr uint32_t HEAR_HAS_CUE        = 14;  // 0/1
    static constexpr uint32_t HEAR_DISTANCE_NORM  = 15;
    static constexpr uint32_t HEAR_DIR_FORWARD    = 16;
    static constexpr uint32_t HEAR_DIR_RIGHT      = 17;
    static constexpr uint32_t HEAR_AGE_NORM       = 18;

    // Tactical memory [19..23]
    static constexpr uint32_t LAST_DAMAGE_AGE     = 19;  // time since last hit, [0,1]
    static constexpr uint32_t LAST_DAMAGE_DIR_FWD = 20;
    static constexpr uint32_t LAST_DAMAGE_DIR_RT  = 21;
    static constexpr uint32_t ABILITY_COOLDOWN    = 22;  // primary ability [0,1]
    static constexpr uint32_t MODE_ONEHOT         = 23;  // quantized mode enum/4

    static constexpr uint32_t TOTAL_DIM = 24;
}

// Creature action layout
namespace CreatureAction
{
    static constexpr uint32_t MOVE_FORWARD  = 0;  // [-1, 1]
    static constexpr uint32_t MOVE_RIGHT    = 1;  // [-1, 1]
    static constexpr uint32_t TURN_RATE     = 2;  // [-1, 1]
    static constexpr uint32_t ATTACK_INTENT = 3;  // [0, 1] fire when > 0.5
    static constexpr uint32_t FLEE_INTENT   = 4;  // [0, 1]
    static constexpr uint32_t ABILITY_INTENT = 5; // [0, 1]
    static constexpr uint32_t TOTAL_DIM = 6;
}

// Vehicle action indices (maps to FInputTensor)
namespace VehicleAction
{
    static constexpr uint32_t THROTTLE = 0;    // [0, 1]
    static constexpr uint32_t BRAKE = 1;       // [0, 1]
    static constexpr uint32_t STEERING = 2;    // [-1, 1]
    static constexpr uint32_t HANDBRAKE = 3;   // [0, 1]
    static constexpr uint32_t TOTAL_DIM = 4;
}

} // namespace Cortex
