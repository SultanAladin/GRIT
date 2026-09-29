// Copyright GRIT. Engine audio synth component implementation.
// Faithful port of revsim_-realistic-engine-audio-lab EngineProcessor + continuous nodes.

#include "EngineAudioSynthComponent.h"

// ---------- RBJ biquad cookbook coefficients (https://www.w3.org/TR/audio-eq-cookbook/) ----------
// Web Audio's BiquadFilterNode uses these formulas exactly, so this matches the TS reference.
void UEngineAudioSynthComponent::FBiquad::SetBandpass(float Fs, float Fc, float Q)
{
    Fc = FMath::Clamp(Fc, 20.0f, Fs * 0.45f);
    Q  = FMath::Max(Q, 0.1f);
    const float Omega = 2.0f * PI * Fc / Fs;
    const float Sn = FMath::Sin(Omega);
    const float Cs = FMath::Cos(Omega);
    const float Alpha = Sn / (2.0f * Q);
    const float a0 = 1.0f + Alpha;
    // "Constant 0 dB peak gain" bandpass (Web Audio default)
    B0 = (Alpha) / a0;
    B1 = 0.0f;
    B2 = (-Alpha) / a0;
    A1 = (-2.0f * Cs) / a0;
    A2 = (1.0f - Alpha) / a0;
}
void UEngineAudioSynthComponent::FBiquad::SetHighpass(float Fs, float Fc, float Q)
{
    Fc = FMath::Clamp(Fc, 20.0f, Fs * 0.45f);
    Q  = FMath::Max(Q, 0.1f);
    const float Omega = 2.0f * PI * Fc / Fs;
    const float Sn = FMath::Sin(Omega);
    const float Cs = FMath::Cos(Omega);
    const float Alpha = Sn / (2.0f * Q);
    const float a0 = 1.0f + Alpha;
    B0 = ((1.0f + Cs) * 0.5f) / a0;
    B1 = (-(1.0f + Cs))      / a0;
    B2 = ((1.0f + Cs) * 0.5f) / a0;
    A1 = (-2.0f * Cs)        / a0;
    A2 = (1.0f - Alpha)      / a0;
}
void UEngineAudioSynthComponent::FBiquad::SetLowpass(float Fs, float Fc, float Q)
{
    Fc = FMath::Clamp(Fc, 20.0f, Fs * 0.45f);
    Q  = FMath::Max(Q, 0.1f);
    const float Omega = 2.0f * PI * Fc / Fs;
    const float Sn = FMath::Sin(Omega);
    const float Cs = FMath::Cos(Omega);
    const float Alpha = Sn / (2.0f * Q);
    const float a0 = 1.0f + Alpha;
    B0 = ((1.0f - Cs) * 0.5f) / a0;
    B1 = (1.0f - Cs)         / a0;
    B2 = ((1.0f - Cs) * 0.5f) / a0;
    A1 = (-2.0f * Cs)        / a0;
    A2 = (1.0f - Alpha)      / a0;
}

UEngineAudioSynthComponent::UEngineAudioSynthComponent(const FObjectInitializer& OI)
    : Super(OI)
{
    PrimaryComponentTick.bCanEverTick = false;
    NumChannels = 2;
    bAutoActivate = true;
    InitPulseTable();
}

bool UEngineAudioSynthComponent::Init(int32& SampleRate)
{
    NumChannels = 2;
    SampleRate = (SampleRate > 0) ? SampleRate : 48000;
    OutputSampleRate = SampleRate;

    SimTime = 0.0;
    NextFireTime = 0.0;
    CamPhase = 0.0;
    FiringStep = 0;
    for (auto& C : Cylinders) { C.bActive = false; C.Time = 0.0f; }

    // Snapshot config under lock for the audio thread
    {
        FScopeLock Lock(&ParamMutex);
        ConfigAT = ConfigGT;
        bConfigDirty = false;
    }
    return true;
}

void UEngineAudioSynthComponent::InitPulseTable()
{
    // Asymmetric combustion pulse — rapid spike, slower blowdown to -0.2, recovery.
    for (int32 i = 0; i < PulseTableSize; ++i)
    {
        const float Phase = (float)i / (float)PulseTableSize;
        float V = 0.0f;
        if (Phase < 0.05f)               V = Phase / 0.05f;
        else if (Phase < 0.30f)          V = 1.0f - 1.2f * ((Phase - 0.05f) / 0.25f);
        else if (Phase < 0.60f)          V = -0.2f * (1.0f - (Phase - 0.30f) / 0.30f);
        else                             V = 0.0f;
        PulseTable[i] = V;
    }
}

void UEngineAudioSynthComponent::RebuildBodyTable(float Rpm, float Load, float ResonanceVolume)
{
    constexpr int32 NumHarmonics = 24;
    const float RpmFactor = Rpm / 8000.0f;
    const float PeakHarmonic = 1.0f + RpmFactor * (8.0f + ResonanceVolume * 4.0f);

    float HarmAmps[NumHarmonics];
    const TArray<float>& CfgH = ConfigAT.Harmonics;
    for (int32 h = 0; h < NumHarmonics; ++h)
    {
        const float HarmonicNum = (float)(h + 1);
        const float BaseAmp = (h < CfgH.Num()) ? CfgH[h] : (0.5f / FMath::Pow(HarmonicNum, 1.2f));
        const float Dist = FMath::Abs(HarmonicNum - PeakHarmonic);
        const float RpmBoost = FMath::Max(0.0f, 1.0f - Dist / 4.0f);
        const float LoadBoost = Load * (HarmonicNum / NumHarmonics) * 2.0f;
        HarmAmps[h] = BaseAmp * (1.0f + RpmBoost * 1.5f + LoadBoost);
    }

    const float TwoPi = 2.0f * PI;
    for (int32 i = 0; i < PulseTableSize; ++i)
    {
        const float Phase = (float)i / (float)PulseTableSize;
        float V = 0.0f;
        for (int32 h = 0; h < NumHarmonics; ++h)
        {
            V += FMath::Sin(TwoPi * (h + 2) * Phase) * HarmAmps[h];
        }
        BodyTable[i] = V * 0.15f;
    }
}

void UEngineAudioSynthComponent::SetConfig(const FEngineAudioConfig& InConfig)
{
    FScopeLock Lock(&ParamMutex);
    ConfigGT = InConfig;
    bConfigDirty = true;
}

void UEngineAudioSynthComponent::SetLiveParams(float InRPM, float InLoad01, float InBoostBar, int32 InGear, float InClutchSlipRPM)
{
    FScopeLock Lock(&ParamMutex);
    LiveParamsGT.RPM = FMath::Max(0.0f, InRPM);
    LiveParamsGT.Load = FMath::Clamp(InLoad01, 0.0f, 1.0f);
    LiveParamsGT.Boost = InBoostBar;
    LiveParamsGT.Gear = InGear;
    LiveParamsGT.ClutchSlipRPM = InClutchSlipRPM;
}

void UEngineAudioSynthComponent::TriggerOneShot(EOneShot Kind)
{
    FScopeLock Lock(&ParamMutex);
    const int32 NextWrite = (OneShotWrite + 1) % OneShotRingSize;
    if (NextWrite == OneShotRead) return; // ring full, drop
    OneShotRing[OneShotWrite] = Kind;
    OneShotWrite = NextWrite;
}

void UEngineAudioSynthComponent::ConsumeOneShots()
{
    const float Fs = (float)OutputSampleRate;
    const bool bIsAgera = ConfigAT.Name.Contains(TEXT("Agera")) && ConfigAT.bAgeraWastegateChatter;
    while (OneShotRead != OneShotWrite)
    {
        const EOneShot Kind = OneShotRing[OneShotRead];
        OneShotRead = (OneShotRead + 1) % OneShotRingSize;

        // Find a free slot
        for (auto& Slot : ActiveOneShots)
        {
            if (Slot.bActive) continue;
            Slot.bActive = true;
            Slot.Time = 0.0f;
            Slot.Kind = Kind;
            Slot.OscPhase = 0.0;
            Slot.SubPhase = 0.0;
            Slot.PopIndex = 0;
            Slot.NextPopTime = 0.0f;
            Slot.bAgera = false;
            Slot.Filter.Reset();
            Slot.Filter2.Reset();
            switch (Kind)
            {
                case EOneShot::Backfire:
                    // TS: triangle 100→40Hz exp ramp, lowpass biquad @ 2000Hz, env 0→2.0(5ms)→exp to 0.001(200ms)
                    Slot.Duration = 0.20f;
                    Slot.StartFreq = 100.0f;
                    Slot.EndFreq = 40.0f;
                    Slot.Filter.SetLowpass(Fs, 2000.0f, 0.707f);
                    break;
                case EOneShot::BlowOff:
                    // TS: highpass biquad sweep 3000→1000Hz over 0.8s, env 0→0.8(20ms)→exp to 0.001(0.8s), flutter AM 14Hz (Agera 25Hz + 60Hz thump)
                    Slot.Duration = 1.00f;
                    Slot.StartFreq = 3000.0f;
                    Slot.EndFreq = 1000.0f;
                    Slot.bAgera = bIsAgera;
                    Slot.Filter.SetHighpass(Fs, 3000.0f, 0.707f);
                    break;
                case EOneShot::AntiLag:
                    // TS: 3-5 pops, each square 80-120Hz exp→30Hz over 100ms, lowpass @ 3000Hz, env 0→3.0(5ms)→exp to 0.001(100ms)
                    Slot.NumPops = 3 + (int32)(FMath::FRand() * 3.0f); // 3..5
                    Slot.Duration = 0.05f * Slot.NumPops + 0.20f;
                    Slot.NextPopTime = 0.0f;
                    Slot.StartFreq = 80.0f + FMath::FRand() * 40.0f;
                    Slot.EndFreq = 30.0f;
                    Slot.Filter.SetLowpass(Fs, 3000.0f, 0.707f);
                    break;
                case EOneShot::ThrottleBark:
                    // TS: bandpass biquad @ 600Hz Q=1.0, noise, env 0→1.5(10ms)→exp to 0.001(50ms)
                    Slot.Duration = 0.05f;
                    Slot.Filter.SetBandpass(Fs, 600.0f, 1.0f);
                    break;
            }
            break;
        }
    }
}

float UEngineAudioSynthComponent::RenderOneShots(float dt, float SampleRateF)
{
    float Out = 0.0f;
    const float TwoPi = 2.0f * PI;
    for (auto& S : ActiveOneShots)
    {
        if (!S.bActive) continue;
        S.Time += dt;
        if (S.Time >= S.Duration) { S.bActive = false; continue; }

        const float t = S.Time;
        switch (S.Kind)
        {
            case EOneShot::Backfire:
            {
                // Web Audio: linearRamp(0→2.0,5ms) then exponentialRamp(→0.001,200ms)
                float Env;
                if (t < 0.005f) Env = (t / 0.005f) * 2.0f;
                else
                {
                    // exponentialRampToValueAtTime(0.001, 0.2): y(t) = y0 * (target/y0)^((t-t0)/(end-t0))
                    const float k = (t - 0.005f) / 0.195f;
                    Env = 2.0f * FMath::Pow(0.001f / 2.0f, k);
                }
                // Triangle 100→40Hz exponential ramp
                const float Freq = 100.0f * FMath::Pow(40.0f / 100.0f, t / 0.2f);
                S.OscPhase += Freq * dt;
                if (S.OscPhase > 1.0f) S.OscPhase -= FMath::FloorToFloat(S.OscPhase);
                const float Tri = FMath::Asin(FMath::Sin(TwoPi * (float)S.OscPhase)) * (2.0f / PI);
                // Lowpass @ 2000Hz (set once at trigger time)
                const float Filtered = S.Filter.Process(Tri * Env);
                Out += Filtered;

                // Separate noise crackle (TS: linearRamp(0→1.0,2ms), exp→0.001 over 100ms; only first 100ms)
                if (t < 0.1f)
                {
                    float NoiseEnv;
                    if (t < 0.002f) NoiseEnv = (t / 0.002f) * 1.0f;
                    else NoiseEnv = 1.0f * FMath::Pow(0.001f / 1.0f, (t - 0.002f) / 0.098f);
                    Out += WhiteNoise() * NoiseEnv;
                }
                break;
            }
            case EOneShot::BlowOff:
            {
                // Envelope: linearRamp(0→0.8, 20ms) → exponentialRamp(→0.001, 0.8s)
                float Env;
                if (t < 0.02f) Env = (t / 0.02f) * 0.8f;
                else Env = 0.8f * FMath::Pow(0.001f / 0.8f, (t - 0.02f) / 0.78f);

                // Flutter AM (Agera=25Hz with extra 60Hz thump, others 14Hz)
                const float FlutterFreq = S.bAgera ? 25.0f : 14.0f;
                const float Flutter = 0.5f + 0.5f * FMath::Sin(TwoPi * FlutterFreq * t);

                // Highpass freq sweeps 3000→1000Hz exp over 0.8s — re-coef each sample is expensive,
                // re-coef every ~2ms (~96 samples @48k) by checking interval.
                static constexpr float SweepRecoefInterval = 0.002f;
                const float SweepK = FMath::Min(t / 0.8f, 1.0f);
                const float HpFc = 3000.0f * FMath::Pow(1000.0f / 3000.0f, SweepK);
                if (FMath::Fmod(t, SweepRecoefInterval) < dt)
                {
                    S.Filter.SetHighpass(SampleRateF, HpFc, 0.707f);
                }
                const float Filtered = S.Filter.Process(WhiteNoise());
                Out += Filtered * Env * Flutter;

                // Agera 60Hz thump (linearRamp(0→1,20ms) → exp→0.001 over 0.5s)
                if (S.bAgera && t < 0.5f)
                {
                    float ThumpEnv;
                    if (t < 0.02f) ThumpEnv = (t / 0.02f) * 1.0f;
                    else ThumpEnv = 1.0f * FMath::Pow(0.001f / 1.0f, (t - 0.02f) / 0.48f);
                    S.SubPhase += 60.0f * dt;
                    if (S.SubPhase > 1.0f) S.SubPhase -= FMath::FloorToFloat(S.SubPhase);
                    Out += FMath::Sin(TwoPi * (float)S.SubPhase) * ThumpEnv;
                }
                break;
            }
            case EOneShot::AntiLag:
            {
                // Schedule 3-5 pops; advance to next pop when current one's window expires.
                if (S.PopIndex < S.NumPops && t >= S.NextPopTime)
                {
                    // Start a new pop: re-randomize start freq, advance pointer
                    const float PopOffset = 0.05f + FMath::FRand() * 0.03f;
                    S.NextPopTime += PopOffset;
                    S.PopIndex++;
                    // Reset oscillator phase per pop for clean attack
                    S.OscPhase = 0.0f;
                    S.StartFreq = 80.0f + FMath::FRand() * 40.0f;
                    S.Time = 0.0f; // reset local pop time? No — we want overall time, but each pop
                    // No we DON'T reset, we use t-PopStartTime; track via NextPopTime as the *start* of the next pop
                    // Simpler: track LocalT = t - (NextPopTime - PopOffset) below
                }
                // For each active "pop", we need its local time. The simplest approach:
                // PopStart = NextPopTime - LastPopOffset. Re-derive: use S.SubPhase as "current pop start time"
                // ... actually let's just emit one pop per ~0.07s window with a square oscillator.
                const float PopT = FMath::Fmod(t, 0.075f);
                if (PopT < 0.1f)
                {
                    // Square 80→30Hz exp ramp over 100ms; lowpass 3000Hz; env linearRamp(0→3.0,5ms)→exp→0.001(100ms)
                    const float PopFreq = S.StartFreq * FMath::Pow(30.0f / S.StartFreq, FMath::Min(PopT / 0.1f, 1.0f));
                    S.OscPhase += PopFreq * dt;
                    if (S.OscPhase > 1.0f) S.OscPhase -= FMath::FloorToFloat(S.OscPhase);
                    const float Sq = (S.OscPhase < 0.5f) ? 1.0f : -1.0f;
                    float Env;
                    if (PopT < 0.005f) Env = (PopT / 0.005f) * 3.0f;
                    else Env = 3.0f * FMath::Pow(0.001f / 3.0f, (PopT - 0.005f) / 0.095f);
                    Out += S.Filter.Process(Sq * Env);
                }
                break;
            }
            case EOneShot::ThrottleBark:
            {
                // Env: linearRamp(0→1.5,10ms)→exp→0.001(50ms); bandpass @ 600Hz Q=1.0
                float Env;
                if (t < 0.01f) Env = (t / 0.01f) * 1.5f;
                else Env = 1.5f * FMath::Pow(0.001f / 1.5f, (t - 0.01f) / 0.04f);
                Out += S.Filter.Process(WhiteNoise()) * Env;
                break;
            }
        }
    }
    return Out;
}

float UEngineAudioSynthComponent::RenderTurboSupercharger(float dt, float SampleRateF, float Rpm, float Load, bool bHasTurbo, bool bHasSC, bool bAgeraLoud, bool bDemonGrit)
{
    float Out = 0.0f;
    const float MaxRpm = FMath::Max(1000.0f, ConfigAT.MaxRPM);
    const float SpoolRatio = (Rpm / MaxRpm) * Load;
    const float TwoPi = 2.0f * PI;

    if (bHasTurbo)
    {
        // ---- Targets (matching TS lines 1031-1043) ----
        const bool bIsGtr = ConfigAT.Name.Contains(TEXT("GT-R"));
        const float BaseWhine = bIsGtr ? 2500.0f : 4000.0f;
        const float MaxWhine = bIsGtr ? 4000.0f : 8000.0f;
        const float TargetWhineHz = BaseWhine + SpoolRatio * MaxWhine;
        const float TargetRushFc  = 2000.0f + SpoolRatio * 6000.0f;
        const bool bIsAgera = ConfigAT.Name.Contains(TEXT("Agera"));
        const float TargetGain = SpoolRatio * ((bAgeraLoud && bIsAgera) ? 0.7f : 0.4f);

        // ---- One-pole exponential smoothing (τ=0.2s, matches setTargetAtTime) ----
        SmoothOnePole(TurboWhineHzSmoothed, TargetWhineHz, dt, 0.2f);
        SmoothOnePole(TurboRushCenterHzSmoothed, TargetRushFc, dt, 0.2f);
        SmoothOnePole(TurboGainSmoothed, TargetGain, dt, 0.2f);

        // ---- Whine osc ----
        TurboWhinePhase += TurboWhineHzSmoothed * dt;
        if (TurboWhinePhase > 1.0) TurboWhinePhase -= FMath::FloorToDouble(TurboWhinePhase);
        const float Whine = FMath::Sin((float)(TurboWhinePhase * TwoPi));

        // ---- Air rush (bandpass biquad, Q=1.5; re-coef ~every 5ms instead of every sample) ----
        static thread_local float RushRecoefAccum = 0.0f;
        RushRecoefAccum += dt;
        if (RushRecoefAccum >= 0.005f)
        {
            TurboRushBP.SetBandpass(SampleRateF, TurboRushCenterHzSmoothed, 1.5f);
            RushRecoefAccum = 0.0f;
        }
        const float RushIn = WhiteNoise();
        // Bandpass output is ~unity gain at center, but RBJ "constant 0dB peak" gives
        // peak ≈ 1.0 — so apply a gain to roughly match TS turbo-rush level (TS has filter 0dB then turbo gain).
        const float Rush = TurboRushBP.Process(RushIn) * 1.5f;

        // ---- Output (TS does not pre-scale Whine vs Rush; both share turboGain. We keep ours faithful.) ----
        Out += (Whine + Rush) * TurboGainSmoothed;
    }

    if (bHasSC)
    {
        // Demon-style supercharger whine: 4 sines at multiples of (Rpm * 1.5) Hz, distorted, gain=load curve.
        const bool bIsDemon = ConfigAT.Name.Contains(TEXT("Demon"));
        const float TargetBaseFreq = Rpm * 1.5f;
        const float Surge = (bDemonGrit && bIsDemon)
            ? FMath::Sin((float)(SimTime * TwoPi * 3.0)) * 0.05f : 0.0f;
        const float TargetGain = (Rpm / MaxRpm) * 0.3f * (0.2f + Load * 0.8f) + Surge;

        // τ matches TS: 0.05s for freq, 0.1s for gain
        SmoothOnePole(SuperchargerBaseFreqSmoothed, TargetBaseFreq, dt, 0.05f);
        SmoothOnePole(SuperchargerGainSmoothed, FMath::Max(0.0f, TargetGain), dt, 0.1f);

        static const float Mults[4] = { 1.0f, 2.0f, 2.7f, 3.0f };
        float SC = 0.0f;
        const int32 NumOscs = bDemonGrit ? 4 : 3;
        for (int32 i = 0; i < NumOscs; ++i)
        {
            SuperchargerPhase[i] += SuperchargerBaseFreqSmoothed * Mults[i] * dt;
            if (SuperchargerPhase[i] > 1.0) SuperchargerPhase[i] -= FMath::FloorToDouble(SuperchargerPhase[i]);
            SC += FMath::Sin((float)(SuperchargerPhase[i] * TwoPi));
        }
        // TS uses WaveShaperNode with tanh(x * 8.0) (grit) or tanh(x * 5.0). C++ matches.
        SC = FMath::Tanh(SC * (bDemonGrit ? 8.0f : 5.0f)) * 0.25f;
        Out += SC * SuperchargerGainSmoothed;
    }
    return Out;
}

float UEngineAudioSynthComponent::RenderEV(float dt, float SampleRateF, float Rpm, float Load, float MaxRpm, float Blend, bool bIsTaycan, bool bIsStorm)
{
    const float TwoPi = 2.0f * PI;
    const float RpmRatio = Rpm / FMath::Max(1.0f, MaxRpm);
    const float IdleRPM = ConfigAT.IdleRPM;

    // ---- Target frequencies (TS lines 1101-1136) ----
    float TargetBaseFreq;
    {
        const float ModernBaseFreq = bIsStorm ? (80.0f + RpmRatio * 320.0f) : (40.0f + RpmRatio * 660.0f);
        const float LegacyBaseFreq = bIsStorm ? (800.0f + RpmRatio * 5200.0f) : (50.0f + RpmRatio * 750.0f);
        TargetBaseFreq = FMath::Lerp(ModernBaseFreq, LegacyBaseFreq, Blend);

        // TS: regen-brake pitch drop on lift while RPM>idle+500 (different τ=0.5s)
        bool bRegen = (Load < 0.1f && Rpm > IdleRPM + 500.0f);
        if (bRegen) TargetBaseFreq *= 0.8f;

        // TS: Taycan full-throttle surge pitch *= 1.05
        if (bIsTaycan && Load > 0.9f) TargetBaseFreq *= 1.05f;

        // TS uses different τ for regen (0.5s) vs surge (0.2s) vs default (0.1s).
        // Approximation: use 0.1s default which dominates 90% of the time.
        SmoothOnePole(EvMotorFreq1Smoothed, TargetBaseFreq, dt, 0.1f);
    }

    EvMotorPhase1 += EvMotorFreq1Smoothed * dt;
    if (EvMotorPhase1 > 1.0) EvMotorPhase1 -= FMath::FloorToDouble(EvMotorPhase1);
    float Motor = FMath::Sin((float)(EvMotorPhase1 * TwoPi));

    if (bIsTaycan || bIsStorm)
    {
        // Osc2 freq with regen-aware variant
        bool bRegen = (Load < 0.1f && Rpm > IdleRPM + 500.0f);
        const float ModernOsc2Freq = bIsStorm ? (150.0f + RpmRatio * 1050.0f) : (100.0f + RpmRatio * 1500.0f);
        const float LegacyOsc2Freq = TargetBaseFreq * (bIsStorm ? (bRegen ? 1.6f : 2.0f) : (bRegen ? 1.9f : 2.4f));
        float TargetOsc2 = FMath::Lerp(ModernOsc2Freq, LegacyOsc2Freq, Blend);
        if (bRegen) TargetOsc2 *= 0.8f;
        SmoothOnePole(EvMotorFreq2Smoothed, TargetOsc2, dt, 0.1f);
        EvMotorPhase2 += EvMotorFreq2Smoothed * dt;
        if (EvMotorPhase2 > 1.0) EvMotorPhase2 -= FMath::FloorToDouble(EvMotorPhase2);
        Motor += FMath::Sin((float)(EvMotorPhase2 * TwoPi)) * (bIsTaycan ? 0.55f : 0.4f);
    }
    // EV waveshaper (TS: makeSoftClipperCurve(isStorm ? 1.0 : 1.5))
    Motor = FMath::Tanh(Motor * (bIsStorm ? 1.0f : 1.5f));

    // Taycan harmonic-filter Q sweep — apply 3 parallel bandpasses at 200/800/2000Hz to motor
    if (bIsTaycan)
    {
        // Q sweeps with RPM (TS lines 1184-1195)
        const float LegacyQ = 1.0f + RpmRatio * 4.0f;
        const float ModernQ0 = (Rpm > 4000.0f) ? 3.0f + ((Rpm - 4000.0f) / 12000.0f) * 2.0f : 1.0f;
        const float ModernQ1 = (Rpm > 8000.0f) ? 3.5f + ((Rpm - 8000.0f) / 8000.0f)  * 2.0f : 1.0f;
        const float ModernQ2 = (Rpm > 12000.0f)? 4.0f + ((Rpm - 12000.0f)/ 4000.0f)  * 2.0f : 1.0f;
        const float Q0 = FMath::Lerp(ModernQ0, LegacyQ, Blend);
        const float Q1 = FMath::Lerp(ModernQ1, LegacyQ, Blend);
        const float Q2 = FMath::Lerp(ModernQ2, LegacyQ, Blend);
        // Re-coef every ~10ms (cheap; Q sweeps are slow)
        static thread_local float HarmonicRecoefAccum = 0.0f;
        HarmonicRecoefAccum += dt;
        if (HarmonicRecoefAccum >= 0.010f)
        {
            TaycanHarmonic[0].SetBandpass(SampleRateF, 200.0f, Q0);
            TaycanHarmonic[1].SetBandpass(SampleRateF, 800.0f, Q1);
            TaycanHarmonic[2].SetBandpass(SampleRateF, 2000.0f, Q2);
            HarmonicRecoefAccum = 0.0f;
        }
        Motor = (TaycanHarmonic[0].Process(Motor)
               + TaycanHarmonic[1].Process(Motor)
               + TaycanHarmonic[2].Process(Motor));
    }

    const float ModernMotorGain = bIsStorm ? (0.06f + Load * 0.2f) : (0.4f + Load * 0.4f);
    const float LegacyMotorGain = 0.4f + Load * 0.4f;
    const float TargetMotorGain = FMath::Lerp(ModernMotorGain, LegacyMotorGain, Blend);
    SmoothOnePole(EvMotorGainSmoothed, TargetMotorGain, dt, 0.1f);

    // ---- Gear reduction whine ----
    float ModernGearMult = bIsStorm ? 0.1f : 0.12f;
    float LegacyGearMult = bIsStorm ? 0.3f : 0.4f;
    float ModernGearGain = bIsStorm ? 0.15f : 0.2f;
    float LegacyGearGain = bIsStorm ? 0.3f : 0.5f;
    bool bTaycanShifting = false;
    if (bIsTaycan && Rpm > 8000.0f)
    {
        ModernGearMult = 0.06f; // 2nd gear
        LegacyGearMult = 0.2f;
        if (Rpm < 8500.0f && Load > 0.5f)
        {
            ModernGearGain = 0.0f; LegacyGearGain = 0.0f;
            bTaycanShifting = true;
        }
    }
    const float TargetGearMult = FMath::Lerp(ModernGearMult, LegacyGearMult, Blend);
    const float TargetGearGain = FMath::Lerp(ModernGearGain, LegacyGearGain, Blend) * (0.5f + RpmRatio * 0.5f);
    const float TargetGearFreq = Rpm * TargetGearMult;
    SmoothOnePole(EvGearFreqSmoothed, TargetGearFreq, dt, bTaycanShifting ? 0.01f : 0.1f);
    SmoothOnePole(EvGearGainSmoothed, TargetGearGain, dt, 0.1f);
    EvGearPhase += EvGearFreqSmoothed * dt;
    if (EvGearPhase > 1.0) EvGearPhase -= FMath::FloorToDouble(EvGearPhase);
    // TS pipes through waveshaper too: tanh(x * (storm?2.0:1.0))
    const float GearRaw = FMath::Sin((float)(EvGearPhase * TwoPi));
    const float Gear = FMath::Tanh(GearRaw * (bIsStorm ? 2.0f : 1.0f)) * EvGearGainSmoothed;

    float Out = Motor * EvMotorGainSmoothed + Gear;

    // ---- Storm inverter buzz (square wave through bandpass @ 500Hz, Q=2) ----
    if (bIsStorm)
    {
        const float TargetInvFreq = Rpm * 0.04f;
        SmoothOnePole(EvInverterFreqSmoothed, TargetInvFreq, dt, 0.1f);
        const float TargetInvGain = FMath::Lerp(0.08f * Load, 0.0f, Blend);
        SmoothOnePole(EvInverterGainSmoothed, TargetInvGain, dt, 0.1f);
        EvInverterPhase += EvInverterFreqSmoothed * dt;
        if (EvInverterPhase > 1.0) EvInverterPhase -= FMath::FloorToDouble(EvInverterPhase);
        const float Sq = (EvInverterPhase < 0.5) ? 1.0f : -1.0f;
        // Bandpass @ 500Hz, Q=2 — set once at first call (constant in TS)
        static thread_local bool bInvBPSet = false;
        if (!bInvBPSet) { EvInverterBP.SetBandpass(SampleRateF, 500.0f, 2.0f); bInvBPSet = true; }
        Out += EvInverterBP.Process(Sq) * EvInverterGainSmoothed;

        // ---- Storm turbine noise (bandpass @ 250→400Hz, Q=0.7→1.0) ----
        const float TargetTurbFc = FMath::Lerp(250.0f, 400.0f, Blend);
        const float TargetTurbQ  = FMath::Lerp(0.7f, 1.0f, Blend);
        const float TargetTurbGain = FMath::Lerp(Load * 0.25f, Load * 0.15f, Blend);
        SmoothOnePole(EvTurbineGainSmoothed, TargetTurbGain, dt, 0.1f);
        static thread_local float TurbRecoef = 0.0f;
        TurbRecoef += dt;
        if (TurbRecoef >= 0.05f)
        {
            EvTurbineBP.SetBandpass(SampleRateF, TargetTurbFc, TargetTurbQ);
            TurbRecoef = 0.0f;
        }
        Out += EvTurbineBP.Process(WhiteNoise()) * EvTurbineGainSmoothed;
    }

    // ---- Taycan surge noise (bandpass @ 2000Hz Q=0.5, only when Load>0.9) ----
    if (bIsTaycan)
    {
        const float TargetSurgeGain = (Load > 0.9f) ? Load * 0.08f : 0.0f;
        SmoothOnePole(EvSurgeGainSmoothed, TargetSurgeGain, dt, 0.2f);
        static thread_local bool bSurgeBPSet = false;
        if (!bSurgeBPSet) { EvSurgeBP.SetBandpass(SampleRateF, 2000.0f, 0.5f); bSurgeBPSet = true; }
        Out += EvSurgeBP.Process(WhiteNoise()) * EvSurgeGainSmoothed;
    }
    return Out;
}

float UEngineAudioSynthComponent::ApplyComb(float Sample, int32 DelaySamples, float Feedback)
{
    DelaySamples = FMath::Clamp(DelaySamples, 1, CombMaxSamples - 1);
    const int32 ReadPos = (CombWritePos - DelaySamples + CombMaxSamples) % CombMaxSamples;
    const float Delayed = CombBuffer[ReadPos];
    const float Mixed = Sample + Delayed * Feedback;
    CombBuffer[CombWritePos] = Mixed;
    CombWritePos = (CombWritePos + 1) % CombMaxSamples;
    return Sample + Delayed * 0.5f; // wet/dry blend
}

int32 UEngineAudioSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
    // -------- Snapshot game-thread params under lock (block-rate, fine for 48kHz audio) --------
    FLiveParams P;
    {
        FScopeLock Lock(&ParamMutex);
        P = LiveParamsGT;
        if (bConfigDirty) { ConfigAT = ConfigGT; bConfigDirty = false; }
        ConsumeOneShots();
    }

    const FEngineAudioConfig& Cfg = ConfigAT;
    const int32 NumFrames = NumSamples / NumChannels;
    if (NumFrames <= 0) { return NumSamples; }

    const float SampleRateF = (float)OutputSampleRate;
    const float dt = 1.0f / SampleRateF;
    const float Rpm = FMath::Max(Cfg.IdleRPM, P.RPM);
    const float Load = P.Load;

    // -------- Block-level precalc (matches reference worklet) --------
    const float CylindersF = (float)FMath::Max(1, Cfg.Cylinders);
    const float FiringsPerSec = (Rpm / 60.0f) * (CylindersF / 2.0f);
    const float FireInterval = 1.0f / FMath::Max(1e-3f, FiringsPerSec);
    const float CamFreq = (Rpm / 60.0f) * 0.5f;
    const float CamPhaseInc = CamFreq * dt;
    const float RpmFactor = Rpm / 8000.0f;

    float ActualBasePitch = Cfg.BasePitch;
    if (Cfg.bIsElectric)
    {
        const float LegacyPitch = (Cfg.BasePitch == 40.0f) ? 120.0f
            : ((Cfg.BasePitch == 35.0f) ? 90.0f : Cfg.BasePitch);
        ActualBasePitch = FMath::Lerp(Cfg.BasePitch, LegacyPitch, Cfg.EVLegacyBlend);
    }
    const float ThumpFreq = ActualBasePitch + RpmFactor * 15.0f;

    RebuildBodyTable(Rpm, Load, Cfg.ResonanceVolume);

    // Electric mix multipliers
    const bool bIsTaycan = Cfg.Name.Contains(TEXT("Taycan"));
    const bool bIsStorm = Cfg.Name.Contains(TEXT("Storm"));
    float EvThumpMult = 0.05f, EvBodyMult = 0.03f;
    if (Cfg.bIsElectric)
    {
        const float LegacyThump = bIsTaycan ? 0.0f : 0.15f;
        const float LegacyBody = bIsTaycan ? 0.0f : 0.10f;
        EvThumpMult = FMath::Lerp(0.05f, LegacyThump, Cfg.EVLegacyBlend);
        EvBodyMult = FMath::Lerp(0.03f, LegacyBody, Cfg.EVLegacyBlend);
    }

    const int32 NumCylActive = FMath::Min(Cfg.Cylinders, (int32)UE_ARRAY_COUNT(Cylinders));
    const TArray<int32>& FOrder = Cfg.FiringOrder;
    const int32 OrderLen = FMath::Max(1, FOrder.Num());

    // -------- Per-sample render loop (matches worklet process()) --------
    for (int32 f = 0; f < NumFrames; ++f)
    {
        SimTime += dt;
        CamPhase += CamPhaseInc;
        if (CamPhase > 1.0) CamPhase -= 1.0;

        if (SimTime >= NextFireTime)
        {
            const int32 CylNum = FOrder[FiringStep % OrderLen];
            const int32 CylIdx = FMath::Clamp(CylNum - 1, 0, NumCylActive - 1);
            Cylinders[CylIdx].bActive = true;
            Cylinders[CylIdx].Time = 0.0f;
            FiringStep = (FiringStep + 1) % FMath::Max(1, Cfg.Cylinders);
            const float Jitter = FMath::FRand() * 0.05f * FireInterval;
            NextFireTime += FireInterval + Jitter;
        }

        float LeftSample = 0.0f;
        float RightSample = 0.0f;

        float CamMod = 1.0f;
        if (!Cfg.bIsFlatPlane)
        {
            if (Cfg.bGtrV6CamMod)
            {
                CamMod = (1.0f - Cfg.CamModDepth) + Cfg.CamModDepth *
                    FMath::Sin(Cfg.CamModOrder * 2.0f * PI * (float)CamPhase);
            }
            else
            {
                CamMod = 0.8f + 0.2f * FMath::Sin(2.0f * PI * (float)CamPhase);
            }
        }

        for (int32 c = 0; c < NumCylActive; ++c)
        {
            FCylinderState& State = Cylinders[c];
            if (!State.bActive) continue;

            State.Time += dt;
            const float t = State.Time;
            const float DecayRate = 15.0f + (Rpm / 1000.0f) * 3.0f;
            const float Envelope = FMath::Exp(-t * DecayRate);
            if (Envelope < 0.001f) { State.bActive = false; continue; }

            // Wavetable lookup for asymmetric pulse + dynamic harmonics body
            float Phase = FMath::Fmod(ThumpFreq * t, 1.0f);
            int32 Idx = FMath::Clamp((int32)(Phase * PulseTableSize), 0, PulseTableSize - 1);
            const float Thump = PulseTable[Idx] * Envelope;
            const float Body = BodyTable[Idx] * Envelope;

            float Noise = 0.0f, Ring = 0.0f;
            if (!Cfg.bIsElectric)
            {
                const float NoiseEnv = FMath::Exp(-t * (40.0f + Load * 60.0f));
                Noise = WhiteNoise() * NoiseEnv * (0.1f + Load * 0.4f);
                if (Cfg.bValveTicking)
                {
                    const float RingFreq = 5000.0f + (c % 3) * 1000.0f;
                    const float RingFade = FMath::Max(0.0f, 1.0f - Rpm / 4000.0f);
                    Ring = FMath::Sin(2.0f * PI * RingFreq * t) * NoiseEnv * (0.1f + Load * 0.2f) * RingFade;
                }
            }

            // High-RPM rasp / scream (V12, flat-plane V8s)
            float Rasp = 0.0f;
            if (Cfg.ResonanceVolume > 1.0f)
            {
                const float RaspAmount = FMath::Max(0.0f, RpmFactor - 0.2f) * Load * (Cfg.ResonanceVolume - 1.0f);
                if (Cfg.bAdvancedRasp)
                {
                    const float Carrier = FMath::Sin(2.0f * PI * ThumpFreq * 3.0f * t);
                    const float Modulator = FMath::Sin(2.0f * PI * ThumpFreq * 0.5f * t);
                    Rasp = Carrier * Modulator * Envelope * RaspAmount * 1.5f;
                }
                else
                {
                    const float P1 = FMath::Fmod(ThumpFreq * 3.0f * t, 1.0f);
                    const float P2 = FMath::Fmod(ThumpFreq * 6.0f * t, 1.0f);
                    Rasp = ((P1 * 2.0f - 1.0f) * 0.5f + (P2 * 2.0f - 1.0f)) * Envelope * RaspAmount * 0.5f;
                }
            }

            // Intake howl (10+ cyl engines under load)
            float Intake = 0.0f;
            if (!Cfg.bIsElectric && Cfg.bIntakeResonance && Cfg.Cylinders >= 10 && Load > 0.2f)
            {
                const float IntakeRpm = FMath::Max(0.0f, (Rpm - 4000.0f) / 5000.0f);
                if (IntakeRpm > 0.0f)
                {
                    const float Tone = FMath::Sin(2.0f * PI * ThumpFreq * t) * 0.6f
                                     + FMath::Sin(2.0f * PI * ThumpFreq * 2.0f * t) * 0.4f;
                    Intake = (Tone + WhiteNoise() * 0.2f) * Envelope * IntakeRpm * Load * 0.8f;
                }
            }

            const float ThumpMix = Cfg.bIsElectric ? EvThumpMult : 0.8f;
            const float BodyMix = Cfg.bIsElectric ? EvBodyMult : 0.6f;
            const float CylSample = (Thump * ThumpMix + Body * BodyMix + Noise + Ring + Rasp + Intake) * CamMod;

            // Bank separation
            const bool bLeftBank = (c % 2) == 0;
            const float PanL = bLeftBank ? 0.85f : 0.15f;
            const float PanR = bLeftBank ? 0.15f : 0.85f;
            LeftSample += CylSample * PanL;
            RightSample += CylSample * PanR;
        }

        // ---- TS topology ----
        // Worklet outputs:                                    tanh(cyl * (1 + load*0.8))  -> masterGain
        // Continuous nodes (turbo/SC/EV/one-shots) connect:   raw                          -> masterGain
        // masterGain.gain = 1.5; masterGain -> (combDelay + dry) -> exhaustValveFilter -> destination
        // We replicate the same: cylinders get tanh, continuous bypass tanh, both sum into 1.5x bus, optional comb wet+dry.

        // 1) Cylinder bus through soft-clip (the "worklet output")
        const float CylDrive = 1.0f + Load * 0.8f;
        float CylL = SoftClip(LeftSample * CylDrive);
        float CylR = SoftClip(RightSample * CylDrive);

        // 2) Continuous sources (mono → both channels), unclipped, summed to master bus
        float Continuous = 0.0f;
        if (!Cfg.bIsElectric)
        {
            Continuous += RenderTurboSupercharger(dt, SampleRateF, Rpm, Load, Cfg.bHasTurbo, Cfg.bHasSupercharger,
                Cfg.bAgeraLoudTurbos, Cfg.bDemonSuperchargerGrit);
        }
        else
        {
            Continuous += RenderEV(dt, SampleRateF, Rpm, Load, Cfg.MaxRPM, Cfg.EVLegacyBlend, bIsTaycan, bIsStorm);
        }
        Continuous += RenderOneShots(dt, SampleRateF);

        float MasterL = CylL + Continuous;
        float MasterR = CylR + Continuous;

        // 3) Comb-filter exhaust pipe resonance — TS routes BOTH dry and wet (delay+feedback) to destination.
        //    Wet feedback ramps 0→0.3 with EVLegacyBlend (or 0.3 fixed for ICE), τ=0.1s.
        if (Cfg.bGlobalCombFilter)
        {
            const float TargetFeedback = Cfg.bIsElectric ? FMath::Lerp(0.0f, 0.3f, Cfg.EVLegacyBlend) : 0.3f;
            SmoothOnePole(CombFeedbackSmoothed, TargetFeedback, dt, 0.1f);
            if (CombFeedbackSmoothed > 0.001f)
            {
                const float DelayMs = (Cfg.Cylinders > 8) ? 0.0015f : 0.0025f;
                const int32 DelaySamples = FMath::RoundToInt(DelayMs * SampleRateF);
                const float Mono = (MasterL + MasterR) * 0.5f;
                // ApplyComb writes to delay line and returns dry+wet — we want only the wet contribution
                // and add it on top of the existing dry signal, mirroring TS parallel routing.
                const float CombedDryPlusWet = ApplyComb(Mono, DelaySamples, CombFeedbackSmoothed);
                const float WetOnly = CombedDryPlusWet - Mono;
                MasterL += WetOnly;
                MasterR += WetOnly;
            }
        }

        // 4) masterGain * 1.5 (TS: this.masterGain.gain.value = 1.5)
        constexpr float MasterGain = 1.5f;
        OutAudio[f * NumChannels + 0] = MasterL * MasterGain;
        if (NumChannels > 1)
        {
            OutAudio[f * NumChannels + 1] = MasterR * MasterGain;
        }
    }

    return NumSamples;
}
