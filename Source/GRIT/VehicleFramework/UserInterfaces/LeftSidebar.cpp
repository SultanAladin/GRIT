#include "LeftSidebar.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"

ULeftSidebar::ULeftSidebar(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ULeftSidebar::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind button click delegates
    if (ExteriorButton)
    {
        ExteriorButton->OnButtonClickedDelegate.AddDynamic(this, &ULeftSidebar::OnExteriorClicked);
    }

    if (InteriorButton)
    {
        InteriorButton->OnButtonClickedDelegate.AddDynamic(this, &ULeftSidebar::OnInteriorClicked);
    }

    if (WheelsButton)
    {
        WheelsButton->OnButtonClickedDelegate.AddDynamic(this, &ULeftSidebar::OnWheelsClicked);
    }

    if (PackagesButton)
    {
        PackagesButton->OnButtonClickedDelegate.AddDynamic(this, &ULeftSidebar::OnPackagesClicked);
    }

    if (AccessoriesButton)
    {
        AccessoriesButton->OnButtonClickedDelegate.AddDynamic(this, &ULeftSidebar::OnAccessoriesClicked);
    }

    if (SummaryButton)
    {
        SummaryButton->OnButtonClickedDelegate.AddDynamic(this, &ULeftSidebar::OnSummaryClicked);
    }
}

void ULeftSidebar::NativeDestruct()
{
    // Clean up delegates to prevent memory leaks
    if (ExteriorButton)
    {
        ExteriorButton->OnButtonClickedDelegate.RemoveAll(this);
    }

    if (InteriorButton)
    {
        InteriorButton->OnButtonClickedDelegate.RemoveAll(this);
    }

    if (WheelsButton)
    {
        WheelsButton->OnButtonClickedDelegate.RemoveAll(this);
    }

    if (PackagesButton)
    {
        PackagesButton->OnButtonClickedDelegate.RemoveAll(this);
    }

    if (AccessoriesButton)
    {
        AccessoriesButton->OnButtonClickedDelegate.RemoveAll(this);
    }

    if (SummaryButton)
    {
        SummaryButton->OnButtonClickedDelegate.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void ULeftSidebar::OnExteriorClicked()
{
    // Broadcast C++ delegate
    OnExteriorClickedDelegate.Broadcast();
    
    // Call Blueprint event
    OnExteriorClickedBP();

    // Debug message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.0f,
            FColor::Cyan,
            TEXT("Exterior clicked!")
        );
    }
}

void ULeftSidebar::OnInteriorClicked()
{
    // Broadcast C++ delegate
    OnInteriorClickedDelegate.Broadcast();
    
    // Call Blueprint event
    OnInteriorClickedBP();

    // Debug message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.0f,
            FColor::Cyan,
            TEXT("Interior clicked!")
        );
    }
}

void ULeftSidebar::OnWheelsClicked()
{
    // Broadcast C++ delegate
    OnWheelsClickedDelegate.Broadcast();
    
    // Call Blueprint event
    OnWheelsClickedBP();

    // Debug message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.0f,
            FColor::Cyan,
            TEXT("Wheels clicked!")
        );
    }
}

void ULeftSidebar::OnPackagesClicked()
{
    // Broadcast C++ delegate
    OnPackagesClickedDelegate.Broadcast();
    
    // Call Blueprint event
    OnPackagesClickedBP();

    // Debug message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.0f,
            FColor::Cyan,
            TEXT("Packages clicked!")
        );
    }
}

void ULeftSidebar::OnAccessoriesClicked()
{
    // Broadcast C++ delegate
    OnAccessoriesClickedDelegate.Broadcast();
    
    // Call Blueprint event
    OnAccessoriesClickedBP();

    // Debug message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.0f,
            FColor::Cyan,
            TEXT("Accessories clicked!")
        );
    }
}

void ULeftSidebar::OnSummaryClicked()
{
    // Broadcast C++ delegate
    OnSummaryClickedDelegate.Broadcast();
    
    // Call Blueprint event
    OnSummaryClickedBP();

    // Debug message
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.0f,
            FColor::Cyan,
            TEXT("Summary clicked!")
        );
    }
}