// Copyright GRIT. Engine audio synth component.
// Direct port of the EngineProcessor AudioWorklet from the RevSim reference.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "EngineAudioConfig.h"
#include "HAL/CriticalSection.h"
#include "EngineAudioSynthComponent.generated.h"

/**
 * USynthComponent that generates engine audio sample-by-sample on the audio render
 * thread. Mirrors the reference Web Audio worklet: cylinder firing scheduler,
 * asymmetric pulse + dynamic harmonics wavetables, intake howl, exhaust rasp,
 * cam modulation, plus continuous turbo/supercharger/EV motor synthesis fused
 * into the same render pass (no separate node graph required).
 *
 * Parameters are updated from the game thread via SetLiveParams() / SetConfig().
 * One-shot transients (backfire, BOV, anti-lag) are queued via TriggerOneShot().
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class GRIT_API UEngineAudioSynthComponent : public USynthComponent
{
    GENERATED_BODY()

public:
    UEngineAudioSynthComponent(const FObjectInitializer& OI);

    /** Engine config (cylinders, harmonics, etc.). Pushed atomically to the audio thread. */
    void SetConfig(const FEngineAudioConfig& InConfig);

    /** Per-frame live parameters from the vehicle solver. */
    void SetLiveParams(float InRPM, float InLoad01, float InBoostBar, int32 InGear, float InClutchSlipRPM);

    enum class EOneShot : uint8
    {
        Backfire,
        BlowOff,
        AntiLag,
        ThrottleBark,
    };
    void TriggerOneShot(EOneShot Kind);

protected:
    // USynthComponent
    virtual bool Init(int32& SampleRate) override;
    virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
    // ---------- Game-thread → audio-thread param block (lock-free double buffer) ----------
    struct FLiveParams
    {
        float RPM = 800.0f;
        float Load = 0.0f;
        float Boost = 0.0f;
        int32 Gear = 2;
        float ClutchSlipRPM = 0.0f;
    };
    FLiveParams LiveParamsGT;
    FCriticalSection ParamMutex; // single-writer/single-reader; 60Hz vs audio block boundary

    // ---------- Config (immutable while playing; swap under lock between blocks) ----------
    FEngineAudioConfig ConfigGT;
    FEngineAudioConfig ConfigAT; // copy used by audio thread
    bool bConfigDirty = false;

    // ---------- One-shot ring (single-producer GT, single-consumer audio) ----------
    static constexpr int32 OneShotRingSize = 16;
    EOneShot OneShotRing[OneShotRingSize];
    int32 OneShotWrite = 0;
    int32 OneShotRead = 0;

    // ---------- Audio-thread persistent state ----------
    int32 OutputSampleRate = 48000;

    struct FCylinderState { bool bActive = false; float Time = 0.0f; };
    FCylinderState Cylinders[16];
    int32 FiringStep = 0;
    double SimTime = 0.0;
    double NextFireTime = 0.0;
    double CamPhase = 0.0;

    // Wavetables (regenerated per audio render block; cheap)
    static constexpr int32 PulseTableSize = 512;
    float PulseTable[PulseTableSize];
    float BodyTable[PulseTableSize];

    // Continuous oscillator phases (turbo whine, supercharger, EV motor)
    double TurboWhinePhase = 0.0;
    double SuperchargerPhase[4] = { 0,0,0,0 };
    double EvMotorPhase1 = 0.0;
    double EvMotorPhase2 = 0.0;
    double EvGearPhase = 0.0;
    double EvInverterPhase = 0.0;

    // -------- Biquad filter state (one struct per active filter slot) --------
    // Direct Form II Transposed: y = b0*x + z1; z1 = b1*x - a1*y + z2; z2 = b2*x - a2*y
    struct FBiquad
    {
        float B0 = 1.f, B1 = 0.f, B2 = 0.f, A1 = 0.f, A2 = 0.f;
        float Z1 = 0.f, Z2 = 0.f;
        FORCEINLINE float Process(float x)
        {
            const float y = B0 * x + Z1;
            Z1 = B1 * x - A1 * y + Z2;
            Z2 = B2 * x - A2 * y;
            return y;
        }
        void Reset() { Z1 = 0.f; Z2 = 0.f; }
        void SetBandpass(float Fs, float Fc, float Q);  // RBJ cookbook constant-skirt-gain bandpass
        void SetHighpass(float Fs, float Fc, float Q);  // RBJ cookbook highpass (Q≈0.707 default)
        void SetLowpass (float Fs, float Fc, float Q);  // RBJ cookbook lowpass
    };

    // Smoothed turbo parameters (~tau=0.2s, matches TS setTargetAtTime(...,0.2))
    float TurboWhineHzSmoothed = 4000.0f;
    float TurboRushCenterHzSmoothed = 2000.0f;
    float TurboGainSmoothed = 0.0f;

    // Smoothed supercharger parameters (~tau=0.05s for freq, 0.1s for gain)
    float SuperchargerBaseFreqSmoothed = 0.0f;
    float SuperchargerGainSmoothed = 0.0f;

    // Smoothed comb-feedback (EV blend lerp 0→0.3, τ=0.1s)
    float CombFeedbackSmoothed = 0.3f;

    // Smoothed EV continuous params (τ=0.1s)
    float EvMotorFreq1Smoothed = 0.0f;
    float EvMotorFreq2Smoothed = 0.0f;
    float EvMotorGainSmoothed  = 0.0f;
    float EvGearFreqSmoothed   = 0.0f;
    float EvGearGainSmoothed   = 0.0f;
    float EvInverterFreqSmoothed = 0.0f;
    float EvInverterGainSmoothed = 0.0f;
    float EvTurbineGainSmoothed  = 0.0f;
    float EvSurgeGainSmoothed    = 0.0f;

    // Bandpass filters for noise sources (allocated once per audio thread)
    FBiquad TurboRushBP;     // 2k→8kHz sweep, Q=1.5
    FBiquad EvInverterBP;    // 500Hz, Q=2.0 (Storm)
    FBiquad EvTurbineBP;     // 250→400Hz, Q=0.7→1.0 (Storm)
    FBiquad EvSurgeBP;       // 2000Hz Q=0.5 (Taycan)
    FBiquad TaycanHarmonic[3]; // 200/800/2000Hz, Q sweeps with RPM (Taycan)

    // Comb-filter delay line (exhaust pipe resonance)
    static constexpr int32 CombMaxSamples = 1024; // ~21ms @48k
    float CombBuffer[CombMaxSamples] = {};
    int32 CombWritePos = 0;

    // -------- One-shot envelope state --------
    // Each active one-shot owns its own dedicated filter (sweep across the lifetime).
    struct FOneShotState
    {
        bool bActive = false;
        float Time = 0.0f;
        float Duration = 0.0f;
        EOneShot Kind = EOneShot::Backfire;
        FBiquad Filter;     // sweep filter (highpass for BOV, lowpass for backfire/anti-lag, bandpass for bark)
        FBiquad Filter2;    // optional secondary filter (anti-lag pop fundamental sweep)
        float StartFreq = 0.0f;     // Hz at t=0
        float EndFreq = 0.0f;       // Hz at t=Duration (exponential interp)
        float OscPhase = 0.0;       // for backfire triangle / anti-lag square sweep
        float SubPhase = 0.0;       // for Agera 60Hz BOV thump
        bool bAgera = false;        // BOV variant
        int32 PopIndex = 0;         // for anti-lag (3-5 pops)
        float NextPopTime = 0.0f;   // for anti-lag scheduler
        int32 NumPops = 4;          // anti-lag total pops
    };
    static constexpr int32 MaxOneShots = 8;
    FOneShotState ActiveOneShots[MaxOneShots];

    // ---------- Helpers ----------
    void InitPulseTable();
    void RebuildBodyTable(float Rpm, float Load, float ResonanceVolume);
    static FORCEINLINE float SoftClip(float x) { return FMath::Tanh(x); }
    static FORCEINLINE float WhiteNoise() { return FMath::FRand() * 2.0f - 1.0f; }

    void ConsumeOneShots();
    float RenderOneShots(float dt, float SampleRateF);
    float RenderTurboSupercharger(float dt, float SampleRateF, float Rpm, float Load, bool bHasTurbo, bool bHasSC, bool bAgeraLoud, bool bDemonGrit);
    float RenderEV(float dt, float SampleRateF, float Rpm, float Load, float MaxRpm, float Blend, bool bIsTaycan, bool bIsStorm);
    float ApplyComb(float Sample, int32 DelaySamples, float Feedback);

    // One-pole exponential smoothing toward Target with time-constant Tau.
    // Equivalent to Web Audio setTargetAtTime: y += (target-y) * (1 - exp(-dt/tau)).
    static FORCEINLINE void SmoothOnePole(float& State, float Target, float dt, float Tau)
    {
        if (Tau <= 0.f) { State = Target; return; }
        const float Alpha = 1.0f - FMath::Exp(-dt / Tau);
        State += (Target - State) * Alpha;
    }
};
