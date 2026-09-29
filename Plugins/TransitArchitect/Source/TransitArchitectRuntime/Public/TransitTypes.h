// TransitTypes.h — Enums, profile presets, and resolved profiles for transit infrastructure
#pragma once

#include "CoreMinimal.h"
#include "TransitTypes.generated.h"

// ── Enums ───────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class ETransitPreset : uint8
{
	Street    UMETA(DisplayName = "Street"),
	Avenue    UMETA(DisplayName = "Avenue"),
	Alley     UMETA(DisplayName = "Alley"),
	Narrow    UMETA(DisplayName = "Narrow"),
	Highway   UMETA(DisplayName = "Highway"),
};

UENUM(BlueprintType)
enum class ETransitFamily : uint8
{
	Road    UMETA(DisplayName = "Road"),
	Bridge  UMETA(DisplayName = "Bridge"),
	Rail    UMETA(DisplayName = "Rail"),
};

UENUM(BlueprintType)
enum class ETransitCapMode : uint8
{
	Flat  UMETA(DisplayName = "Flat"),
	None  UMETA(DisplayName = "None"),
	Round UMETA(DisplayName = "Round"),
};

UENUM(BlueprintType)
enum class ETransitJunctionType : uint8
{
	Auto       UMETA(DisplayName = "Auto"),
	T          UMETA(DisplayName = "T"),
	X          UMETA(DisplayName = "X"),
	Y          UMETA(DisplayName = "Y"),
	Roundabout UMETA(DisplayName = "Roundabout"),
};

UENUM(BlueprintType)
enum class ETransitRebuildMode : uint8
{
	Dirty UMETA(DisplayName = "Dirty Only"),
	Full  UMETA(DisplayName = "Full Network"),
};

// ── Material section indices ────────────────────────────────────────────────

namespace TransitMaterial
{
	constexpr int32 RoadSurface  = 0;
	constexpr int32 Curb         = 1;
	constexpr int32 Pavement     = 2;
	constexpr int32 SectionCount = 3;
}

// ── Profile preset (static data) ────────────────────────────────────────────

USTRUCT(BlueprintType)
struct TRANSITARCHITECTRUNTIME_API FTransitProfilePreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ProfileId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Label;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RoadWidth       = 800.0f;   // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PavementLeft    = 200.0f;   // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PavementRight   = 200.0f;   // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurbHeight      = 18.0f;    // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurbWidth       = 22.0f;    // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LaneCount       = 2;
};

// ── Resolved profile (computed from preset + overrides) ─────────────────────

USTRUCT(BlueprintType)
struct TRANSITARCHITECTRUNTIME_API FResolvedTransitProfile
{
	GENERATED_BODY()

	UPROPERTY() FString ProfileId;
	UPROPERTY() FString Label;
	UPROPERTY() float RoadWidth      = 800.0f;
	UPROPERTY() float PavementLeft   = 200.0f;
	UPROPERTY() float PavementRight  = 200.0f;
	UPROPERTY() float CurbHeight     = 18.0f;
	UPROPERTY() float CurbWidth      = 22.0f;
	UPROPERTY() int32 LaneCount      = 2;

	float RoadHalfWidth() const { return RoadWidth * 0.5f; }
	float LeftTotalHalfWidth() const { return RoadHalfWidth() + CurbWidth + PavementLeft; }
	float RightTotalHalfWidth() const { return RoadHalfWidth() + CurbWidth + PavementRight; }
};

// ── Junction override (per-node settings) ───────────────────────────────────

USTRUCT(BlueprintType)
struct TRANSITARCHITECTRUNTIME_API FJunctionOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CornerRadiusOverride = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bForceIgnore          = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled              = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETransitJunctionType NodeType = ETransitJunctionType::Auto;
};

// ── Static helpers ──────────────────────────────────────────────────────────

struct TRANSITARCHITECTRUNTIME_API FTransitProfiles
{
	static const TMap<ETransitPreset, FTransitProfilePreset>& GetBuiltinPresets();
	static const FTransitProfilePreset& GetPreset(ETransitPreset Preset);
};
