#pragma once

#include "VehicleSolver.h"
#include "VehicleConstruct.generated.h"

class USpringArmComponent;
class UCameraComponent;


//------------------------------------------------------------------------------
//                                   actor class
//------------------------------------------------------------------------------
UCLASS()
class GRIT_API AVehicleConstruct : public AVehicleSolver
{
    GENERATED_BODY()

public:
    AVehicleConstruct();

    /** Get spring arm component */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    USpringArmComponent* GetSpringArm() const { return SpringArm; }

    /** Get chase camera component */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    UCameraComponent* GetCamera() const { return ChaseCamera; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    USpringArmComponent* SpringArm = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    UCameraComponent* ChaseCamera = nullptr;
};
