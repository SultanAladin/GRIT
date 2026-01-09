#pragma once

#include "CoreMinimal.h"
#include "DropList.h"
#include "DropListSlide.generated.h"

/*====================================================================================================================================
                                                         ANIMATED DROPDOWN WITH SLIDE
======================================================================================================================================*/

/** Extended dropdown with content slide animation */
UCLASS()
class GRIT_API UDropListSlide : public UDropList
{
    GENERATED_BODY()

public:
    UDropListSlide(const FObjectInitializer& ObjectInitializer);

    //------------------------------------------------------------------------------
    // slide animation configuration
    //------------------------------------------------------------------------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    bool bUseSlideAnimation; // [bool] - Enable content slide effect

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropList|Animation")
    float SlideDistance; // [px] - Vertical slide distance

protected:
    virtual void NativeConstruct() override;

private:
    //------------------------------------------------------------------------------
    // slide animation state
    //------------------------------------------------------------------------------
    
    float StartSlideOffset; // [px] - Slide animation start offset
    float TargetSlideOffset; // [px] - Slide animation target offset

    //------------------------------------------------------------------------------
    // overridden animation methods
    //------------------------------------------------------------------------------
    
    /** Start expand animation with slide */
    virtual void StartExpand() override;

    /** Start collapse animation with slide */
    virtual void StartCollapse() override;

    /** Tick animation update with slide */
    virtual void TickAnim() override;

    /** Apply slide transform to content */
    void ApplySlideTransform(float YOffset);

    /** Initialize content transform */
    void InitContentTransform();
};
