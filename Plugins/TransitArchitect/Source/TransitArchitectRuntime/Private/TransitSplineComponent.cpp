// TransitSplineComponent.cpp
#include "TransitSplineComponent.h"

UTransitSplineComponent::UTransitSplineComponent()
{
	// Default: 2 points forming a straight road segment
	SetClosedLoop(false);
}

FResolvedTransitProfile UTransitSplineComponent::ResolveProfile() const
{
	const FTransitProfilePreset& Preset = FTransitProfiles::GetPreset(ProfilePreset);

	FResolvedTransitProfile Resolved;
	Resolved.ProfileId    = Preset.ProfileId;
	Resolved.Label        = Preset.Label;
	Resolved.RoadWidth    = (RoadWidthOverride > 0.0f)    ? RoadWidthOverride    : Preset.RoadWidth;
	Resolved.PavementLeft = (PavementLeftOverride > 0.0f) ? PavementLeftOverride : Preset.PavementLeft;
	Resolved.PavementRight= (PavementRightOverride > 0.0f)? PavementRightOverride: Preset.PavementRight;
	Resolved.CurbHeight   = (CurbHeightOverride > 0.0f)  ? CurbHeightOverride   : Preset.CurbHeight;
	Resolved.CurbWidth    = (CurbWidthOverride > 0.0f)   ? CurbWidthOverride    : Preset.CurbWidth;
	Resolved.LaneCount    = (LaneCountHint > 0)          ? LaneCountHint        : Preset.LaneCount;
	return Resolved;
}
