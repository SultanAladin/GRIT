// Copyright GRIT. Static facade implementation.

#include "EngineAudioSim.h"
#include "EngineAudioSynthComponent.h"
#include "VehicleSolver.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

TMap<TWeakObjectPtr<USceneComponent>, FEngineAudioSim::FRuntime>& FEngineAudioSim::GetRegistry()
{
    static TMap<TWeakObjectPtr<USceneComponent>, FRuntime> Registry;
    return Registry;
}

FEngineAudioSim::FRuntime& FEngineAudioSim::GetOrCreate(USceneComponent* Owner)
{
    auto& Reg = GetRegistry();
    return Reg.FindOrAdd(Owner);
}

FEngineAudioSim::FRuntime* FEngineAudioSim::Find(USceneComponent* Owner)
{
    auto& Reg = GetRegistry();
    return Reg.Find(Owner);
}

UEngineAudioSynthComponent* FEngineAudioSim::GetSynth(USceneComponent* Owner)
{
    if (!Owner) return nullptr;
    if (FRuntime* RT = Find(Owner))
    {
        return RT->Synth.Get();
    }
    return nullptr;
}

void FEngineAudioSim::SetConfig(USceneComponent* Owner, const FEngineAudioConfig& Config)
{
    if (!Owner) return;
    FRuntime& RT = GetOrCreate(Owner);
    UEngineAudioSynthComponent* Synth = RT.Synth.Get();
    if (!Synth)
    {
        AActor* OwnerActor = Owner->GetOwner();
        if (!OwnerActor) return;

        Synth = NewObject<UEngineAudioSynthComponent>(OwnerActor);
        Synth->SetupAttachment(Owner);
        Synth->RegisterComponent();
        RT.Synth = Synth;
    }
    Synth->SetConfig(Config);

    // Cache flags used by SolveAudio's event detection.
    RT.ConfigName = Config.Name;
    RT.bHasTurbo = Config.bHasTurbo;
    RT.RedlineRPM = Config.RedlineRPM;
    RT.bGtrAntiLag = Config.bGtrAntiLag;
    RT.bDemonThrottleBark = Config.bDemonThrottleBark;

    if (!Synth->IsActive())
    {
        Synth->Start();
    }
}

void FEngineAudioSim::SetConfig(USceneComponent* Owner, EPreset Preset)
{
    FEngineAudioConfig Cfg;
    switch (Preset)
    {
        case EPreset::LaFerrari:           Cfg = UEngineAudioPresets::LaFerrari(); break;
        case EPreset::ViperACR:            Cfg = UEngineAudioPresets::ViperACR(); break;
        case EPreset::SrtDemon:            Cfg = UEngineAudioPresets::SrtDemon(); break;
        case EPreset::GtrNismo:            Cfg = UEngineAudioPresets::GtrNismo(); break;
        case EPreset::PorscheSpyder918:    Cfg = UEngineAudioPresets::PorscheSpyder918(); break;
        case EPreset::KoenigseggAgeraRS:   Cfg = UEngineAudioPresets::KoenigseggAgeraRS(); break;
        case EPreset::JacksonStormEV:      Cfg = UEngineAudioPresets::JacksonStormEV(); break;
        case EPreset::TaycanTurbo:         Cfg = UEngineAudioPresets::TaycanTurbo(); break;
    }
    SetConfig(Owner, Cfg);
}

void FEngineAudioSim::SolveAudio(
    USceneComponent* Owner,
    const FVehicleSolverOutput& Out,
    float ThrottleInput)
{
    if (!Owner) return;
    FRuntime* RT = Find(Owner);
    if (!RT) return;
    UEngineAudioSynthComponent* Synth = RT->Synth.Get();
    if (!Synth) return;

    // ----- Derive Load [0..1]: prefer normalized engine torque vs load, fallback to throttle -----
    float Load = FMath::Clamp(ThrottleInput, 0.0f, 1.0f);
    if (Out.EngineTorque_Nm > 1.0f)
    {
        // When power is being delivered, blend torque-based load with throttle for responsiveness.
        const float TorqueLoad = FMath::Clamp(Out.EngineTorque_Nm / 800.0f, 0.0f, 1.0f);
        Load = FMath::Max(Load, TorqueLoad);
    }
    // Reduce load while clutch is heavily slipping (drivetrain disengaged → less audible "lugging")
    if (Out.ClutchSlipRPM > 1500.0f && Out.ClutchEngagement < 0.4f)
    {
        Load *= 0.6f;
    }

    Synth->SetLiveParams(Out.EngineRPM, Load, Out.BoostPressure_bar, Out.CurrentGear, Out.ClutchSlipRPM);

    // ----- Event detection (mirrors EngineSimulator.setParameters() event hooks) -----
    const float PrevThrottle = RT->LastThrottle;
    const int32 PrevGear = RT->LastGear;

    // ----- BOV / anti-lag on throttle lift (turbo only) — matches TS lines 823-830 -----
    // TS: prevThrottle > 0.7 && load < 0.1 && rpm > 3000.   Note: NO boost gate.
    // GT-R config + RPM>4000 → anti-lag instead of BOV.
    if (RT->bHasTurbo && PrevThrottle > 0.7f && ThrottleInput < 0.1f && Out.EngineRPM > 3000.0f)
    {
        const bool bIsGtr = RT->ConfigName.Contains(TEXT("GT-R"));
        if (bIsGtr && RT->bGtrAntiLag && Out.EngineRPM > 4000.0f)
        {
            Synth->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::AntiLag);
        }
        else
        {
            Synth->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::BlowOff);
        }
    }

    // ----- Backfire on lift above 60% redline — TS line 833: 20% chance (Math.random()>0.8) -----
    const bool bSolverBackfire = Out.bBackfireEvent && !RT->bLastBackfireEvent;
    const float RedlineThreshold = FMath::Max(1000.0f, RT->RedlineRPM) * 0.6f;
    const bool bManualBackfire = (PrevThrottle > 0.6f && ThrottleInput < 0.1f
        && Out.EngineRPM > RedlineThreshold && FMath::FRand() > 0.8f);
    if (bSolverBackfire || bManualBackfire)
    {
        Synth->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::Backfire);
    }

    // Solver-driven BOV vent (independent of throttle heuristic; turbo only)
    if (RT->bHasTurbo && Out.bBovVentEvent && !RT->bLastBovEvent)
    {
        Synth->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::BlowOff);
    }

    // ----- Throttle bark — TS line 838: Demon-only, prevThrottle<0.1 && load>0.8 -----
    const double NowSec = FPlatformTime::Seconds();
    const bool bIsDemon = RT->ConfigName.Contains(TEXT("Demon"));
    if (RT->bDemonThrottleBark && bIsDemon
        && PrevThrottle < 0.1f && ThrottleInput > 0.8f
        && (NowSec - RT->LastBarkTime) > 0.25)
    {
        Synth->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::ThrottleBark);
        RT->LastBarkTime = NowSec;
    }

    // Subtle gear-shift backfire on aggressive upshifts under load
    if (Out.CurrentGear > PrevGear && PrevGear >= 2 && ThrottleInput > 0.7f && Out.EngineRPM > 4500.0f)
    {
        if (FMath::FRand() > 0.6f)
        {
            Synth->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::Backfire);
        }
    }

    RT->LastThrottle = ThrottleInput;
    RT->LastGear = Out.CurrentGear;
    RT->bLastBovEvent = Out.bBovVentEvent;
    RT->bLastBackfireEvent = Out.bBackfireEvent;
}

void FEngineAudioSim::TriggerBackfire(USceneComponent* Owner)
{
    if (UEngineAudioSynthComponent* S = GetSynth(Owner)) S->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::Backfire);
}
void FEngineAudioSim::TriggerBlowOff(USceneComponent* Owner)
{
    if (UEngineAudioSynthComponent* S = GetSynth(Owner)) S->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::BlowOff);
}
void FEngineAudioSim::TriggerAntiLag(USceneComponent* Owner)
{
    if (UEngineAudioSynthComponent* S = GetSynth(Owner)) S->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::AntiLag);
}
void FEngineAudioSim::TriggerThrottleBark(USceneComponent* Owner)
{
    if (UEngineAudioSynthComponent* S = GetSynth(Owner)) S->TriggerOneShot(UEngineAudioSynthComponent::EOneShot::ThrottleBark);
}

void FEngineAudioSim::ReleaseFor(USceneComponent* Owner)
{
    if (!Owner) return;
    auto& Reg = GetRegistry();
    if (FRuntime* RT = Reg.Find(Owner))
    {
        if (UEngineAudioSynthComponent* S = RT->Synth.Get())
        {
            S->Stop();
            S->DestroyComponent();
        }
    }
    Reg.Remove(Owner);

    // Also clean up any stale entries whose owners have been destroyed.
    for (auto It = Reg.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
