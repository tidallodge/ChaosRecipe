#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EDamageType.h"
#include "CombatAbilityStruct.h"
#include "CombatTypes.generated.h"

// The core stats every combatant (UPlayerMinion now, enemies later) is built from.
USTRUCT(BlueprintType)
struct FCombatAttributes
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    int32 Health = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    int32 Strength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    int32 Intelligence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    int32 Dexterity = 0;
};

// Everything an ability needs from whoever is using it, so CombatMath::BuildHit works the same for
// a minion or an enemy without knowing which one it's dealing with.
USTRUCT(BlueprintType)
struct FCombatOffense
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 Level = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FCombatAttributes Attributes;

    // Min-max damage per type that Attack abilities build on (e.g. an equipped weapon's local damage).
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    TMap<EDamageType, FIntPoint> WeaponDamage;

    // Attacks per second. Not used per hit; exposed for whatever ends up driving combat timing.
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float AttackRate = 1.f;

    // Percent chance (0-100) for Attack abilities to crit, and the damage multiplier when they do.
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float CritChance = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float CritMultiplier = 1.f;
};

USTRUCT(BlueprintType)
struct FCombatDefense
{
    GENERATED_BODY()

    // Rolled per hit and turned into a physical damage reduction (see CombatMath::ResolveHit).
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FIntPoint PhysicalMitigation = FIntPoint::ZeroValue;

    // Percent chance (0-100) to evade a hit that can be evaded (attacks), rolled per hit.
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FIntPoint EvadeChance = FIntPoint::ZeroValue;

    // Range the overshield pool is rolled from whenever the combatant is restored to full.
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FIntPoint Overshield = FIntPoint::ZeroValue;

    // Percent damage reduction per non-physical damage type. Negative values increase damage taken.
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    TMap<EDamageType, float> Resistances;
};

// One outgoing hit, after the attacker's scaling and crit but before the target's defenses.
USTRUCT(BlueprintType)
struct FCombatHit
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    TMap<EDamageType, float> Damage;

    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool bIsCritical = false;

    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool bCanBeEvaded = true;

    // Subtracted from the target's resistances (not physical mitigation) for this hit only.
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    float ResistancePenetration = 0.f;
};

USTRUCT(BlueprintType)
struct FCombatHitResult
{
    GENERATED_BODY()

    // False when the hit never happened at all (dead attacker or target, no target, level too low, ...).
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bResolved = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bEvaded = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bIsCritical = false;

    // Post-mitigation damage per type, i.e. what got through the target's defenses.
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    TMap<EDamageType, float> DamageTaken;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float OvershieldDamage = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float HealthDamage = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bKilledTarget = false;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCombatTarget : public UInterface
{
    GENERATED_BODY()
};

// Implemented by anything that can be hit: UPlayerMinion now, and the enemy class once it exists, so
// minions and enemies can attack each other without either knowing the other's concrete type.
class CHAOSRECIPE_API ICombatTarget
{
    GENERATED_BODY()

public:
    // Applies an incoming hit to this target's own defenses and health (normally just
    // CombatMath::ResolveHit against its FCombatDefense) and reports what happened.
    virtual FCombatHitResult ApplyHit(const FCombatHit& Hit) = 0;

    virtual bool IsDefeated() const = 0;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCombatant : public UCombatTarget
{
    GENERATED_BODY()
};

// A combat target that can also act on its own (UPlayerMinion, UEnemy), so UBattleManager can run
// both sides of a battle through the same loop.
class CHAOSRECIPE_API ICombatant : public ICombatTarget
{
    GENERATED_BODY()

public:
    virtual FString GetCombatantName() const = 0;

    // Every attack/spell this combatant can pick from right now. Always includes a basic attack.
    virtual TArray<FCombatAbilityStruct> GetBattleAbilities() const = 0;

    // Attacks per second for Attack abilities (spells use their own CastRate instead).
    virtual float GetBattleAttackRate() const = 0;

    virtual FCombatHitResult UseAbility(const FCombatAbilityStruct& Ability, ICombatTarget* Target) = 0;
};

// The damage math shared by every combatant, kept in one place so minions and enemies can't drift apart.
namespace CombatMath
{
    // Picks a value between Range's two ends, inclusive, in whichever order they're stored.
    CHAOSRECIPE_API int32 RollRange(const FIntPoint& Range);

    // The attack every combatant can always fall back on: 100% of its weapon damage, at its attack rate.
    CHAOSRECIPE_API FCombatAbilityStruct MakeBasicAttack();

    // Seconds a combatant has to wait after using Ability before it can act again: one over its rate,
    // which is AttackRate (attacks per second) for attacks and the ability's CastRate for spells.
    CHAOSRECIPE_API float GetAbilityCooldown(const FCombatAbilityStruct& Ability, float AttackRate);

    // Rolls Ability's damage as used by Attacker: attacks start from Attacker's weapon damage (scaled by
    // the ability's WeaponDamageEffectiveness) and use Attacker's crit, spells use only the ability's own
    // damage and crit. Both add the ability's BaseDamage and scale with its ScalingAttribute.
    CHAOSRECIPE_API FCombatHit BuildHit(const FCombatAbilityStruct& Ability, const FCombatOffense& Attacker);

    // Resolves Hit against Defense: an evade roll (if the hit can be evaded), then physical mitigation
    // and resistances per damage type, then the total drains InOutOvershield first and InOutHealth after.
    CHAOSRECIPE_API FCombatHitResult ResolveHit(const FCombatHit& Hit, const FCombatDefense& Defense, float& InOutHealth, float& InOutOvershield);
}
