#pragma once

#include "CoreMinimal.h"
#include "TierClusterBase.h"
#include "Tier1A.generated.h"

UCLASS()
class GRIT_API ATier1A : public ATierClusterBase
{
    GENERATED_BODY()
public:
    ATier1A();
protected:
    virtual int32 GetChildNX() const override { return TierLayout::Tier2NX; }
    virtual int32 GetChildNY() const override { return TierLayout::Tier2NY; }
    virtual FVector2D GetChildCellSizeCm() const override { return FVector2D(TierLayout::Tier2CellX_Cm, TierLayout::Tier2CellY_Cm); }
    virtual TSubclassOf<ATierClusterBase> ChildClassForDensity(ETierDensity InDensity) const override;
};
