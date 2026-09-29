// Copyright GRIT. Static facade for the engine audio simulator.

#pragma once

#include "CoreMinimal.h"
#include "EngineAudioConfig.h"
#include "EngineAudioSim.generated.h"

class UEngineAudioSynthComponent;
class USceneComponent;
struct FVehicleSolverOutput;

/** Engine audio preset selector (BP-friendly UENUM mirrors FEngineAudioSim::EPreset). */
UENUM(BlueprintType)
enum class EEngineAudioPreset : uint8
{
    LaFerrari,
    ViperACR,
    SrtDemon,
    GtrNismo,
    PorscheSpyder918,
    KoenigseggAgeraRS,
    JacksonStormEV,
    TaycanTurbo
};

/**
 * Static utility class — primary entry point for vehicle code.
 *
 * Typical per-vehicle usage:
 *
 *   FEngineAudioSim::SetConfig(VehicleHull, UEngineAudioPresets::LaFerrari());
 *   ...
 *   // Each tick (game thread, after solver output is consumed):
 *   FEngineAudioSim::SolveAudio(VehicleHull, SolverOutput, ThrottleInput);
 *
 *   // On teardown:
 *   FEngineAudioSim::ReleaseFor(VehicleHull);
 *
 * Internally a UEngineAudioSynthComponent is attached to Owner. The facade
 * derives Load, Boost, gear-shift / lift-off / backfire events from the solver
 * output and pushes them to the synth using lock-free param updates.
 */
class GRIT_API FEngineAudioSim
{
public:
    /** Assign or replace the engine config attached to Owner (creates synth on first call). */
    static void SetConfig(USceneComponent* Owner, const FEngineAudioConfig& Config);

    /** Convenience: assign by preset enum. */
    enum class EPreset : uint8
    {
        LaFerrari, ViperACR, SrtDemon, GtrNismo,
        PorscheSpyder918, KoenigseggAgeraRS, JacksonStormEV, TaycanTurbo
    };
    static void SetConfig(USceneComponent* Owner, EPreset Preset);

    /**
     * Per-frame driver. Pushes solver output → synth params, derives event triggers
     * (BOV on lift, backfire on overrun, anti-lag, throttle bark).
     *
     * @param Owner          Scene component to attach the synth to (typically vehicle hull).
     * @param SolverOutput   Latest physics output from VehicleSolver.
     * @param ThrottleInput  Driver throttle pedal [0..1] from the input layer.
     */
    static void SolveAudio(
        USceneComponent* Owner,
        const FVehicleSolverOutput& SolverOutput,
        float ThrottleInput);

    /** Manual one-shot trigger (e.g. from gameplay code). */
    static void TriggerBackfire(USceneComponent* Owner);
    static void TriggerBlowOff(USceneComponent* Owner);
    static void TriggerAntiLag(USceneComponent* Owner);
    static void TriggerThrottleBark(USceneComponent* Owner);

    /** Detach and destroy the synth attached to Owner (if any). */
    static void ReleaseFor(USceneComponent* Owner);

    /** Direct access to the underlying synth (e.g. for spatialization tweaks). May return null. */
    static UEngineAudioSynthComponent* GetSynth(USceneComponent* Owner);

private:
    // Per-owner persistent state for event detection between SolveAudio calls.
    struct FRuntime
    {
        TWeakObjectPtr<UEngineAudioSynthComponent> Synth;
        float LastThrottle = 0.0f;
        int32 LastGear = 2;
        bool bLastBovEvent = false;
        bool bLastBackfireEvent = false;
        double LastBarkTime = 0.0;

        // Cached config flags used by event-detection (mirrors FEngineAudioConfig).
        FString ConfigName;
        bool bHasTurbo = false;
        float RedlineRPM = 7000.0f;
        bool bGtrAntiLag = true;
        bool bDemonThrottleBark = true;
    };
    static FRuntime& GetOrCreate(USceneComponent* Owner);
    static FRuntime* Find(USceneComponent* Owner);
    static TMap<TWeakObjectPtr<USceneComponent>, FRuntime>& GetRegistry();
};
