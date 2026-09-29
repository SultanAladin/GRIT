// Copyright GRIT. Built-in engine audio presets ported from CAR_CONFIGS.

#include "EngineAudioConfig.h"

namespace
{
    static FEngineAudioConfig MakeBase(const TCHAR* Name, int32 Cylinders, float IdleRPM, float MaxRPM, float RedlineRPM, float BasePitch)
    {
        FEngineAudioConfig C;
        C.Name = Name;
        C.Cylinders = Cylinders;
        C.IdleRPM = IdleRPM;
        C.MaxRPM = MaxRPM;
        C.RedlineRPM = RedlineRPM;
        C.BasePitch = BasePitch;
        return C;
    }
}

FEngineAudioConfig UEngineAudioPresets::LaFerrari()
{
    FEngineAudioConfig C = MakeBase(TEXT("Ferrari LaFerrari"), 12, 1000.0f, 9250.0f, 9000.0f, 58.0f);
    C.FiringOrder      = { 1,12,5,8,3,10,6,7,2,11,4,9 };
    C.Harmonics        = { 0.8f, 1.0f, 0.5f, 0.9f, 0.4f, 0.6f, 0.3f, 0.2f };
    C.bIsFlatPlane     = false;
    C.ResonanceVolume  = 2.0f;
    C.CamModOrder      = 6;
    C.CamModDepth      = 0.1f;
    return C;
}

FEngineAudioConfig UEngineAudioPresets::ViperACR()
{
    FEngineAudioConfig C = MakeBase(TEXT("Dodge Viper ACR"), 10, 750.0f, 6200.0f, 6000.0f, 38.0f);
    C.FiringOrder      = { 1,6,5,10,2,7,3,8,4,9 };
    C.Harmonics        = { 1.0f, 0.9f, 0.4f, 0.7f, 0.3f, 0.5f };
    C.ResonanceVolume  = 0.5f;
    C.CamModOrder      = 5;
    C.CamModDepth      = 0.2f;
    return C;
}

FEngineAudioConfig UEngineAudioPresets::SrtDemon()
{
    FEngineAudioConfig C = MakeBase(TEXT("Dodge Challenger SRT Demon"), 8, 800.0f, 6500.0f, 6000.0f, 35.0f);
    C.FiringOrder      = { 1,8,4,3,6,5,7,2 };
    C.Harmonics        = { 1.0f, 0.8f, 0.6f, 0.5f, 0.3f, 0.2f };
    C.bHasSupercharger = true;
    C.ResonanceVolume  = 0.3f;
    C.CamModOrder      = 4;
    C.CamModDepth      = 0.2f;
    return C;
}

FEngineAudioConfig UEngineAudioPresets::GtrNismo()
{
    FEngineAudioConfig C = MakeBase(TEXT("Nissan GT-R Nismo"), 6, 900.0f, 7500.0f, 7000.0f, 48.0f);
    C.FiringOrder      = { 1,4,2,5,3,6 };
    C.Harmonics        = { 1.0f, 0.6f, 0.9f, 0.7f, 0.4f, 0.3f };
    C.bHasTurbo        = true;
    C.ResonanceVolume  = 0.7f;
    C.CamModOrder      = 3;
    C.CamModDepth      = 0.15f;
    return C;
}

FEngineAudioConfig UEngineAudioPresets::PorscheSpyder918()
{
    FEngineAudioConfig C = MakeBase(TEXT("Porsche 918 Spyder"), 8, 1000.0f, 9150.0f, 8700.0f, 65.0f);
    C.FiringOrder      = { 1,8,3,6,4,5,2,7 };
    C.Harmonics        = { 1.0f, 1.4f, 0.6f, 0.8f };
    C.bIsFlatPlane     = true;
    C.ResonanceVolume  = 1.5f;
    C.CamModOrder      = 4;
    C.CamModDepth      = 0.0f;
    return C;
}

FEngineAudioConfig UEngineAudioPresets::KoenigseggAgeraRS()
{
    FEngineAudioConfig C = MakeBase(TEXT("Koenigsegg Agera RS"), 8, 800.0f, 8250.0f, 7800.0f, 58.0f);
    C.FiringOrder      = { 1,5,4,8,2,6,3,7 };
    C.Harmonics        = { 1.2f, 0.5f, 1.1f, 0.4f, 0.9f, 0.3f, 0.7f };
    C.bHasTurbo        = true;
    C.bIsFlatPlane     = true;
    C.ResonanceVolume  = 1.3f;
    C.CamModOrder      = 4;
    C.CamModDepth      = 0.0f;
    return C;
}

FEngineAudioConfig UEngineAudioPresets::JacksonStormEV()
{
    FEngineAudioConfig C = MakeBase(TEXT("Jackson Storm (Next-Gen EV)"), 2, 1500.0f, 15000.0f, 14000.0f, 40.0f);
    C.FiringOrder      = { 1,2 };
    C.Harmonics        = { 0.4f, 0.2f, 0.1f, 0.05f };
    C.bIsFlatPlane     = true;
    C.ResonanceVolume  = 0.0f;
    C.CamModOrder      = 2;
    C.CamModDepth      = 0.0f;
    C.bIsElectric      = true;
    return C;
}

FEngineAudioConfig UEngineAudioPresets::TaycanTurbo()
{
    FEngineAudioConfig C = MakeBase(TEXT("Porsche Taycan Turbo"), 2, 1000.0f, 16000.0f, 15000.0f, 35.0f);
    C.FiringOrder      = { 1,2 };
    C.Harmonics        = { 0.3f, 0.15f, 0.08f, 0.04f };
    C.bIsFlatPlane     = true;
    C.ResonanceVolume  = 0.0f;
    C.CamModOrder      = 2;
    C.CamModDepth      = 0.0f;
    C.bIsElectric      = true;
    return C;
}
