#pragma once

#include "CoreMinimal.h"
#include "TierClusterBase.h"
#include "Tier2A.generated.h"

UCLASS()
class GRIT_API ATier2A : public ATierClusterBase
{
    GENERATED_BODY()
public:
    ATier2A();
protected:
    virtual int32 GetChildNX() const override { return TierLayout::Tier3NX; }
    virtual int32 GetChildNY() const override { return TierLayout::Tier3NY; }
    virtual FVector2D GetChildCellSizeCm() const override { return FVector2D(TierLayout::Tier3CellX_Cm, TierLayout::Tier3CellY_Cm); }
    virtual TSubclassOf<ATierClusterBase> ChildClassForDensity(ETierDensity InDensity) const override;
};
