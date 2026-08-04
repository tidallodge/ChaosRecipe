#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EDamageType.h"
#include "EMitigationType.h"
#include "BaseArmorStruct.generated.h"

UENUM(BlueprintType)
enum class EArmorType : uint8
{
    Head UMETA(DisplayName = "Head"),
    Chest UMETA(DisplayName = "Chest"),
    Gloves UMETA(DisplayName = "Gloves"),
    Boots UMETA(DisplayName = "Boots"),
};

UENUM(BlueprintType)
enum class EArmorSlot : uint8
{
    Head UMETA(DisplayName = "Head"),
    Chest UMETA(DisplayName = "Chest"),
    Gloves UMETA(DisplayName = "Gloves"),
    Boots UMETA(DisplayName = "Boots"),
};

USTRUCT(BlueprintType)
struct FArmorPrimaryBaseDefense
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32 MinDefense;

    UPROPERTY(EditAnywhere)
    int32 MaxDefense;

    UPROPERTY(EditAnywhere)
    EMitigationType MitigationType;
};

USTRUCT(BlueprintType)
struct FArmorSecondaryBaseDefense
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32 MinDefense;

    UPROPERTY(EditAnywhere)
    int32 MaxDefense;

    UPROPERTY(EditAnywhere)
    EMitigationType MitigationType;
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
    FArmorPrimaryBaseDefense ArmorPrimaryBaseDefense;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FArmorSecondaryBaseDefense ArmorSecondaryBaseDefense;
};
