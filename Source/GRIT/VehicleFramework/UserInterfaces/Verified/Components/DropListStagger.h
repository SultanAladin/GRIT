#pragma once

#include "CoreMinimal.h"
#include "DropList.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "DropListStagger.generated.h"

/*====================================================================================================================================
                                                         STAGGERED ITEM ANIMATION
======================================================================================================================================*/

/** Dropdown with staggered individual item animations */
UCLASS()
class GRIT_API UDropListStagger : public UDropList
{
    GENERATED_BODY()

public:
    UDropListStagger(const FObjectInitializer& ObjectInitializer);

    //------------------------------------------------------------------------------
    // stagger animation configuration
    //------------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    bool bUseStaggerAnimation; // [bool] - Enable per-item stagger

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    float StaggerDelay; // [s] - Delay between each item

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    float ItemSlideDistance; // [px] - Individual item slide distance

protected:
    virtual void NativeConstruct() override;

    //------------------------------------------------------------------------------
    // overridden animation methods
    //------------------------------------------------------------------------------
    
    /** Start expand animation with stagger */
    virtual void StartExpand() override;

    /** Start collapse animation with stagger */
    virtual void StartCollapse() override;

    /** Tick animation update with stagger */
    virtual void TickAnim() override;

private:
    //------------------------------------------------------------------------------
    // stagger animation state
    //------------------------------------------------------------------------------
    
    TArray<UUserWidget*> CachedEntryWidgets; // [TArray] - Widget references
    TArray<float> ItemStartTimes; // [s] - Per-item animation start times
    
    //------------------------------------------------------------------------------
    // internal operations
    //------------------------------------------------------------------------------
    
    /** Cache all entry widgets from ListView */
    void CacheEntryWidgets();

    /** Apply transform to individual item */
    void ApplyItemTransform(UUserWidget* Widget, float YOffset);

    /** Initialize item transforms */
    void InitItemTransforms();

    /** Calculate progress for specific item */
    float GetItemProgress(int32 ItemIndex) const;

    /** Reset all item transforms */
    void ResetItemTransforms();
};
