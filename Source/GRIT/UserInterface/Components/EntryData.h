#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EntryData.generated.h"

/*====================================================================================================================================
                                                         ENTRY DATA FOUNDATION
======================================================================================================================================*/

/** Base data container for list entries */
UCLASS(BlueprintType, Abstract)
class GRIT_API UEntryData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    int32 EntryID; // [int32] - Unique entry identifier

    /** Retrieve entry type identifier */
    virtual FName RetrieveEntryType() const { return FName("Base"); }
};

//------------------------------------------------------------------------------
//                          static entry variants
//------------------------------------------------------------------------------

/** Text-only entry */
UCLASS(BlueprintType)
class GRIT_API UTextEntry : public UEntryData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    FText LabelText; // [FText] - Display text content

    virtual FName RetrieveEntryType() const override { return FName("Text"); }
    
    /** Bootstrap text entry */
    void BootstrapText(int32 ID, FText Label) { EntryID = ID; LabelText = Label; }
};

/** Image-only entry */
UCLASS(BlueprintType)
class GRIT_API UImageEntry : public UEntryData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    UTexture2D* IconTexture; // [UTexture2D*] - Display texture asset

    virtual FName RetrieveEntryType() const override { return FName("Image"); }
    
    /** Bootstrap image entry */
    void BootstrapImage(int32 ID, UTexture2D* Icon) { EntryID = ID; IconTexture = Icon; }
};

/** Text and image combined entry */
UCLASS(BlueprintType)
class GRIT_API ULabeledImageEntry : public UEntryData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    FText LabelText; // [FText] - Display text content

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    UTexture2D* IconTexture; // [UTexture2D*] - Display texture asset

    virtual FName RetrieveEntryType() const override { return FName("LabeledImage"); }
    
    /** Bootstrap labeled image entry */
    void BootstrapLabeledImage(int32 ID, FText Label, UTexture2D* Icon) { EntryID = ID; LabelText = Label; IconTexture = Icon; }
};

//------------------------------------------------------------------------------
//                          config blueprint struct
//------------------------------------------------------------------------------

/** Configuration struct for SimpleList entries */
USTRUCT(BlueprintType)
struct FSimpleEntryConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    FText EntryText; // [FText] - Optional text content

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    UTexture2D* EntryTexture; // [UTexture2D*] - Optional texture asset

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    bool bIsButton; // [bool] - Toggle interactive mode

    FSimpleEntryConfig() : EntryTexture(nullptr), bIsButton(false) {}
};

/*====================================================================================================================================
                                                         INTERACTIVE BUTTON ENTRIES
======================================================================================================================================*/

/** Base class for interactive button entries */
UCLASS(Abstract)
class GRIT_API UButtonEntryBase : public UEntryData
{
    GENERATED_BODY()

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnButtonEvent, int32, EntryID);

    UPROPERTY(BlueprintAssignable, Category = "Entry|Events")
    FOnButtonEvent OnHovered; // [Delegate] - Mouse enter broadcast

    UPROPERTY(BlueprintAssignable, Category = "Entry|Events")
    FOnButtonEvent OnUnhovered; // [Delegate] - Mouse leave broadcast

    UPROPERTY(BlueprintAssignable, Category = "Entry|Events")
    FOnButtonEvent OnPressed; // [Delegate] - Mouse down broadcast

    UPROPERTY(BlueprintAssignable, Category = "Entry|Events")
    FOnButtonEvent OnReleased; // [Delegate] - Mouse up broadcast

    UPROPERTY(BlueprintAssignable, Category = "Entry|Events")
    FOnButtonEvent OnClicked; // [Delegate] - Full click broadcast
};

//------------------------------------------------------------------------------
//                          button entry variants
//------------------------------------------------------------------------------

/** Text button entry */
UCLASS(BlueprintType)
class GRIT_API UTextButtonEntry : public UButtonEntryBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    FText LabelText; // [FText] - Display text content

    virtual FName RetrieveEntryType() const override { return FName("TextButton"); }
    
    /** Bootstrap text button entry */
    void BootstrapTextButton(int32 ID, FText Label) { EntryID = ID; LabelText = Label; }
};

/** Image button entry */
UCLASS(BlueprintType)
class GRIT_API UImageButtonEntry : public UButtonEntryBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    UTexture2D* IconTexture; // [UTexture2D*] - Display texture asset

    virtual FName RetrieveEntryType() const override { return FName("ImageButton"); }
    
    /** Bootstrap image button entry */
    void BootstrapImageButton(int32 ID, UTexture2D* Icon) { EntryID = ID; IconTexture = Icon; }
};

/** Text and image button entry */
UCLASS(BlueprintType)
class GRIT_API ULabeledImageButtonEntry : public UButtonEntryBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    FText LabelText; // [FText] - Display text content

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
    UTexture2D* IconTexture; // [UTexture2D*] - Display texture asset

    virtual FName RetrieveEntryType() const override { return FName("LabeledImageButton"); }
    
    /** Bootstrap labeled image button entry */
    void BootstrapLabeledImageButton(int32 ID, FText Label, UTexture2D* Icon) { EntryID = ID; LabelText = Label; IconTexture = Icon; }
};