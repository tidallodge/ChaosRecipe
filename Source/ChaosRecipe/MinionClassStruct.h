#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CombatTypes.h"
#include "MinionClassStruct.generated.h"

UENUM(BlueprintType)
enum class EMinionClass : uint8
{
    Warrior UMETA(DisplayName = "Warrior"),
    Ranger UMETA(DisplayName = "Ranger"),
    Mage UMETA(DisplayName = "Mage"),
};

USTRUCT(BlueprintType)
struct FMinionClassStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion Class")
    EMinionClass MinionClass = EMinionClass::Warrior;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion Class")
    FText ClassName;

    // Stats at level 1, before any equipped items.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion Class")
    FCombatAttributes BaseAttributes;

    // Added to BaseAttributes once for every level past 1.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion Class")
    FCombatAttributes AttributesPerLevel;
};
