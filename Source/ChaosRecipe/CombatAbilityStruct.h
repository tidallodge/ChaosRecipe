#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EDamageType.h"
#include "CombatAbilityStruct.generated.h"

UENUM(BlueprintType)
enum class EAbilityType : uint8
{
    Attack UMETA(DisplayName = "Attack"), // builds on the user's weapon damage and crit, and can be evaded
    Spell UMETA(DisplayName = "Spell"),   // uses only its own damage and crit, and can't be evaded
};

UENUM(BlueprintType)
enum class ECombatAttribute : uint8
{
    None UMETA(DisplayName = "None"),
    Strength UMETA(DisplayName = "Strength"),
    Intelligence UMETA(DisplayName = "Intelligence"),
    Dexterity UMETA(DisplayName = "Dexterity"),
};

// One attack or spell. These are the properties CombatMath::BuildHit/ResolveHit read; a new mechanic
// is a new field here plus handling for it there, so every combatant picks it up at once.
USTRUCT(BlueprintType)
struct FCombatAbilityStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    FText AbilityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    FText AbilityName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    EAbilityType AbilityType = EAbilityType::Attack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    int32 RequiredLevel = 1;

    // Spells: the ability's entire damage. Attacks: added on top of the weapon's damage.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    TMap<EDamageType, FIntPoint> BaseDamage;

    // Attacks only: percent of the weapon's damage this ability deals (100 = full weapon damage).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float WeaponDamageEffectiveness = 100.f;

    // Each point of ScalingAttribute increases this ability's damage by DamagePerAttributePoint percent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    ECombatAttribute ScalingAttribute = ECombatAttribute::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float DamagePerAttributePoint = 0.f;

    // Spells only (attacks use the weapon's): percent chance to crit, and the damage multiplier when they do.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float CritChance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float CritMultiplier = 1.5f;

    // Subtracted from the target's resistances (not physical mitigation) for this ability's hits.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    float ResistancePenetration = 0.f;

    // Spells only (attacks use the user's attack rate): casts per second. In battle, the caster can't act
    // again for 1 / CastRate seconds after casting (see CombatMath::GetAbilityCooldown).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (ClampMin = "0.1"))
    float CastRate = 1.f;
};
