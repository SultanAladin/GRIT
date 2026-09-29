// TransitTypes.cpp
#include "TransitTypes.h"

static TMap<ETransitPreset, FTransitProfilePreset> BuildPresets()
{
	TMap<ETransitPreset, FTransitProfilePreset> Map;

	// All values in centimeters (Blender meters × 100)
	{
		FTransitProfilePreset P;
		P.ProfileId    = TEXT("STREET");
		P.Label        = TEXT("Street");
		P.Description  = TEXT("Default neighborhood road with sidewalks.");
		P.RoadWidth    = 800.0f;
		P.PavementLeft = 200.0f;  P.PavementRight = 200.0f;
		P.CurbHeight   = 18.0f;   P.CurbWidth     = 22.0f;
		P.LaneCount    = 2;
		Map.Add(ETransitPreset::Street, P);
	}
	{
		FTransitProfilePreset P;
		P.ProfileId    = TEXT("AVENUE");
		P.Label        = TEXT("Avenue");
		P.Description  = TEXT("Wider urban road for heavier traffic.");
		P.RoadWidth    = 1400.0f;
		P.PavementLeft = 300.0f;  P.PavementRight = 300.0f;
		P.CurbHeight   = 18.0f;   P.CurbWidth     = 25.0f;
		P.LaneCount    = 4;
		Map.Add(ETransitPreset::Avenue, P);
	}
	{
		FTransitProfilePreset P;
		P.ProfileId    = TEXT("ALLEY");
		P.Label        = TEXT("Alley");
		P.Description  = TEXT("Compact service lane.");
		P.RoadWidth    = 500.0f;
		P.PavementLeft = 120.0f;  P.PavementRight = 120.0f;
		P.CurbHeight   = 12.0f;   P.CurbWidth     = 15.0f;
		P.LaneCount    = 1;
		Map.Add(ETransitPreset::Alley, P);
	}
	{
		FTransitProfilePreset P;
		P.ProfileId    = TEXT("NARROW");
		P.Label        = TEXT("Narrow");
		P.Description  = TEXT("Narrow single-lane road with minimal pavements.");
		P.RoadWidth    = 400.0f;
		P.PavementLeft = 100.0f;  P.PavementRight = 100.0f;
		P.CurbHeight   = 12.0f;   P.CurbWidth     = 15.0f;
		P.LaneCount    = 1;
		Map.Add(ETransitPreset::Narrow, P);
	}
	{
		FTransitProfilePreset P;
		P.ProfileId    = TEXT("HIGHWAY");
		P.Label        = TEXT("Highway");
		P.Description  = TEXT("Multi-lane highway with wide shoulders.");
		P.RoadWidth    = 2000.0f;
		P.PavementLeft = 350.0f;  P.PavementRight = 350.0f;
		P.CurbHeight   = 22.0f;   P.CurbWidth     = 30.0f;
		P.LaneCount    = 6;
		Map.Add(ETransitPreset::Highway, P);
	}

	return Map;
}

const TMap<ETransitPreset, FTransitProfilePreset>& FTransitProfiles::GetBuiltinPresets()
{
	static TMap<ETransitPreset, FTransitProfilePreset> Presets = BuildPresets();
	return Presets;
}

const FTransitProfilePreset& FTransitProfiles::GetPreset(ETransitPreset Preset)
{
	const auto& Map = GetBuiltinPresets();
	const FTransitProfilePreset* Found = Map.Find(Preset);
	if (Found)
	{
		return *Found;
	}
	static FTransitProfilePreset Default = Map[ETransitPreset::Street];
	return Default;
}
