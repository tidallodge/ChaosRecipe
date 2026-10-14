#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ModifierBucketsStruct.h"
#include "BaseJewelryStruct.generated.h"

// Covers both Jewelry and Misc items, which share BaseJewelry_DT (ItemClass in BaseItem_DT tells them apart).
// Names must match the type strings ModifierAssigner::ModifierMatchesItem checks (FValidJewelryTypes/FValidMiscTypes)
UENUM(BlueprintType)
enum class EJewelryType : uint8
{
    Neck UMETA(DisplayName = "Neck"),
    Ring UMETA(DisplayName = "Ring"),
    Belt UMETA(DisplayName = "Belt"),
    Misc UMETA(DisplayName = "Misc"),
};

UENUM(BlueprintType)
enum class EJewelrySlot : uint8
{
    Neck UMETA(DisplayName = "Neck"),
    Ring UMETA(DisplayName = "Ring"),
    Belt UMETA(DisplayName = "Belt"),
    Misc UMETA(DisplayName = "Misc"),
};

USTRUCT(BlueprintType)
struct FBaseJewelryStruct : public FTableRowBase
{
    GENERATED_BODY()

    // A BaseItem_DT ItemId (picked from a dropdown in the data table editor).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (GetOptions = "ChaosRecipe.ItemHandler.GetItemIdOptions"))
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EJewelryType JewelryType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EJewelrySlot JewelrySlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta=(DataTable="/Game/ItemData/ItemModifier_DT.ItemModifier_DT"))
    TArray<FName> ImplicitModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FModifierBuckets ModifierBuckets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 ItemBaseGoldValue;
};
