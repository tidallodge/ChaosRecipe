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

    // Min-max damage per type for this class's basic attack while it has no weapon equipped. An equipped
    // weapon's damage replaces this entirely.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion Class")
    TMap<EDamageType, FIntPoint> Damage;

    // Basic attacks per second while no weapon is equipped. An equipped weapon's attack rate replaces this.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion Class", meta = (ClampMin = "0.1"))
    float AttackRate = 1.f;

    // Attacks/spells this class can pick in battle on top of its basic weapon attack, once the minion
    // reaches each one's RequiredLevel.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion Class")
    TArray<FCombatAbilityStruct> Abilities;
};
