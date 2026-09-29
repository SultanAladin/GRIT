// TransitSplineComponent.h — Spline component with transit corridor profile properties
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "TransitTypes.h"
#include "TransitSplineComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = "Transit Architect", meta = (BlueprintSpawnableComponent))
class TRANSITARCHITECTRUNTIME_API UTransitSplineComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UTransitSplineComponent();

	// ── Profile ─────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile")
	ETransitPreset ProfilePreset = ETransitPreset::Street;

	/** Override road width (cm). 0 = use preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile", meta = (ClampMin = "0"))
	float RoadWidthOverride = 0.0f;

	/** Override left pavement width (cm). 0 = use preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile", meta = (ClampMin = "0"))
	float PavementLeftOverride = 0.0f;

	/** Override right pavement width (cm). 0 = use preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile", meta = (ClampMin = "0"))
	float PavementRightOverride = 0.0f;

	/** Override curb height (cm). 0 = use preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile", meta = (ClampMin = "0"))
	float CurbHeightOverride = 0.0f;

	/** Override curb width (cm). 0 = use preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile", meta = (ClampMin = "0"))
	float CurbWidthOverride = 0.0f;

	/** Override lane count. 0 = use preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Profile", meta = (ClampMin = "0"))
	int32 LaneCountHint = 0;

	// ── Corridor type ───────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Corridor")
	ETransitFamily Family = ETransitFamily::Road;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Corridor")
	ETransitCapMode CapMode = ETransitCapMode::Flat;

	// ── Junction ────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit Architect|Junction")
	float JunctionRadiusBias = 0.0f;

	// ── Methods ─────────────────────────────────────────────────────────

	/** Resolves the effective profile from preset + overrides */
	FResolvedTransitProfile ResolveProfile() const;
};
