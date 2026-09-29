// Copyright GRIT. Realistic engine audio simulator config.
// Ported from revsim_-realistic-engine-audio-lab/src/lib/EngineSimulator.ts (CAR_CONFIGS).

#pragma once

#include "CoreMinimal.h"
#include "EngineAudioConfig.generated.h"

/**
 * Per-engine acoustic configuration. Mirrors EngineConfig from the original
 * Web Audio reference implementation. Values define cylinder count, firing order,
 * harmonic content and per-engine flags consumed by UEngineAudioSynthComponent.
 *
 * NOTE: harmonics array is stored in fixed-size form to keep the struct trivially
 * relocatable for lock-free push to the audio thread. Up to 12 harmonic weights.
 */
USTRUCT(BlueprintType)
struct GRIT_API FEngineAudioConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio", meta = (ClampMin = "1", ClampMax = "16"))
    int32 Cylinders = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    float IdleRPM = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    float MaxRPM = 7000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    float RedlineRPM = 6800.0f;

    /** Firing order: 1-based cylinder indices, length == Cylinders. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    TArray<int32> FiringOrder;

    /** Base frequency [Hz] of a single firing pulse body. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    float BasePitch = 50.0f;

    /** Harmonic weights (1st..Nth). Up to 12 used. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    TArray<float> Harmonics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    bool bHasTurbo = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    bool bHasSupercharger = false;

    /** Flat-plane vs cross-plane crank (affects bank cam-modulation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    bool bIsFlatPlane = false;

    /** > 1.0 enables high-RPM exhaust rasp/scream. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    float ResonanceVolume = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    int32 CamModOrder = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    float CamModDepth = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio")
    bool bIsElectric = false;

    /** EV-only: 0 = modern silent EV, 1 = legacy "fake combustion" blend. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EVLegacyBlend = 0.0f;

    // ------------------- Feature toggles (mirrors EngineSimulator public flags) -------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bValveTicking = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bIntakeResonance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bAdvancedRasp = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bGtrV6CamMod = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bDemonSuperchargerGrit = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bDemonThrottleBark = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bGtrAntiLag = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bAgeraLoudTurbos = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bAgeraWastegateChatter = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine Audio|Toggles")
    bool bGlobalCombFilter = true;
};

/** Built-in presets ported from CAR_CONFIGS in the reference TS. */
UCLASS()
class GRIT_API UEngineAudioPresets : public UObject
{
    GENERATED_BODY()

public:
    /** Identifier mapping to one of the preset configs. */
    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig LaFerrari();

    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig ViperACR();

    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig SrtDemon();

    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig GtrNismo();

    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig PorscheSpyder918();

    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig KoenigseggAgeraRS();

    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig JacksonStormEV();

    UFUNCTION(BlueprintCallable, Category = "Engine Audio")
    static FEngineAudioConfig TaycanTurbo();
};
