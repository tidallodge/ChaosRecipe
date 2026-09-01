#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EDamageType.h"
#include "ModifierBucketsStruct.h"
#include "BaseArmorStruct.generated.h"

UENUM(BlueprintType)
enum class EArmorType : uint8
{
    Head UMETA(DisplayName = "Head"),
    Chest UMETA(DisplayName = "Chest"),
    Gloves UMETA(DisplayName = "Gloves"),
    Boots UMETA(DisplayName = "Boots"),
    Shield UMETA(DisplayName = "Shield"),
};

UENUM(BlueprintType)
enum class EArmorSlot : uint8
{
    Head UMETA(DisplayName = "Head"),
    Chest UMETA(DisplayName = "Chest"),
    Gloves UMETA(DisplayName = "Gloves"),
    Boots UMETA(DisplayName = "Boots"),
    OffHand UMETA(DisplayName = "OffHand"),
};

USTRUCT(BlueprintType)
struct FBaseDefense
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FIntPoint BasePhysicalMitigation;

    UPROPERTY(EditAnywhere)
    FIntPoint BaseEvade;

    UPROPERTY(EditAnywhere)
    FIntPoint BaseOvershield;
};

USTRUCT(BlueprintType)
struct FBaseArmorStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EArmorType ArmorType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EArmorSlot ArmorSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FBaseDefense BaseDefense;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta=(DataTable="/Game/ItemData/ItemModifier_DT.ItemModifier_DT"))
    TArray<FName> ImplicitModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FModifierBuckets ModifierBuckets;
};
