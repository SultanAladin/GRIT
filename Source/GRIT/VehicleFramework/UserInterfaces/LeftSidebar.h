#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HoverButtonV2.h"
#include "Components/VerticalBox.h"
#include "LeftSidebar.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GRIT_API ULeftSidebar : public UUserWidget
{
    GENERATED_BODY()

public:
    ULeftSidebar(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Main container
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* SidebarContainer;

    // Sidebar menu buttons
    UPROPERTY(meta = (BindWidget))
    class UHoverButtonV2* ExteriorButton;

    UPROPERTY(meta = (BindWidget))
    class UHoverButtonV2* InteriorButton;

    UPROPERTY(meta = (BindWidget))
    class UHoverButtonV2* WheelsButton;

    UPROPERTY(meta = (BindWidget))
    class UHoverButtonV2* PackagesButton;

    UPROPERTY(meta = (BindWidget))
    class UHoverButtonV2* AccessoriesButton;

    UPROPERTY(meta = (BindWidget))
    class UHoverButtonV2* SummaryButton;

private:
    // Button click handlers
    UFUNCTION()
    void OnExteriorClicked();

    UFUNCTION()
    void OnInteriorClicked();

    UFUNCTION()
    void OnWheelsClicked();

    UFUNCTION()
    void OnPackagesClicked();

    UFUNCTION()
    void OnAccessoriesClicked();

    UFUNCTION()
    void OnSummaryClicked();

public:
    // Blueprint events for menu clicks
    UFUNCTION(BlueprintImplementableEvent, Category = "Sidebar Events")
    void OnExteriorClickedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Sidebar Events")
    void OnInteriorClickedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Sidebar Events")
    void OnWheelsClickedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Sidebar Events")
    void OnPackagesClickedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Sidebar Events")
    void OnAccessoriesClickedBP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Sidebar Events")
    void OnSummaryClickedBP();

    // C++ delegates for menu clicks
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExteriorClicked);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteriorClicked);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWheelsClicked);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPackagesClicked);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAccessoriesClicked);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSummaryClicked);

    UPROPERTY(BlueprintAssignable, Category = "Sidebar Events")
    FOnExteriorClicked OnExteriorClickedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Sidebar Events")
    FOnInteriorClicked OnInteriorClickedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Sidebar Events")
    FOnWheelsClicked OnWheelsClickedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Sidebar Events")
    FOnPackagesClicked OnPackagesClickedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Sidebar Events")
    FOnAccessoriesClicked OnAccessoriesClickedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Sidebar Events")
    FOnSummaryClicked OnSummaryClickedDelegate;
};