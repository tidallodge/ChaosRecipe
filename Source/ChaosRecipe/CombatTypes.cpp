// CombatTypes
/** Purpose: Shared hit building and hit resolution for every combatant (minions, enemies)
*/

#include "CombatTypes.h"
#include "CombatAbilityStruct.h"

namespace
{
    // Physical reduction = Mitigation / (Mitigation + PhysicalMitigationScaling), so mitigation equal to
    // this value halves physical damage, and every extra point is worth a little less than the last.
    constexpr float PhysicalMitigationScaling = 100.f;
    constexpr float MaxPhysicalReduction = 0.9f;

    // Percent caps, so stacking resistance or evade can never make a combatant fully immune.
    constexpr float MaxResistance = 75.f;
    constexpr float MaxEvadeChance = 75.f;

    // Bounds on attack/cast rates, so a rate of 0 can't stall a combatant forever and a huge one can't
    // make the battle loop spin through thousands of actions in a single tick.
    constexpr float MinActionsPerSecond = 0.1f;
    constexpr float MaxActionsPerSecond = 100.f;

    int32 GetAttributeValue(const FCombatAttributes& Attributes, ECombatAttribute Attribute)
    {
        switch (Attribute)
        {
        case ECombatAttribute::Strength:     return Attributes.Strength;
        case ECombatAttribute::Intelligence: return Attributes.Intelligence;
        case ECombatAttribute::Dexterity:    return Attributes.Dexterity;
        case ECombatAttribute::None:
        default:                             return 0;
        }
    }
}

int32 CombatMath::RollRange(const FIntPoint& Range)
{
    return FMath::RandRange(FMath::Min(Range.X, Range.Y), FMath::Max(Range.X, Range.Y));
}

FCombatAbilityStruct CombatMath::MakeBasicAttack()
{
    FCombatAbilityStruct BasicAttack;
    BasicAttack.AbilityId = FText::FromString(TEXT("basic_attack"));
    BasicAttack.AbilityName = FText::FromString(TEXT("Basic Attack"));
    BasicAttack.AbilityType = EAbilityType::Attack;
    return BasicAttack;
}

float CombatMath::GetAbilityCooldown(const FCombatAbilityStruct& Ability, float AttackRate)
{
    const float ActionsPerSecond = Ability.AbilityType == EAbilityType::Attack ? AttackRate : Ability.CastRate;
    return 1.f / FMath::Clamp(ActionsPerSecond, MinActionsPerSecond, MaxActionsPerSecond);
}

FCombatHit CombatMath::BuildHit(const FCombatAbilityStruct& Ability, const FCombatOffense& Attacker)
{
    const bool bIsAttack = Ability.AbilityType == EAbilityType::Attack;

    FCombatHit Hit;
    Hit.bCanBeEvaded = bIsAttack;
    Hit.ResistancePenetration = Ability.ResistancePenetration;

    auto AddRolledDamage = [&Hit](EDamageType DamageType, const FIntPoint& Range, float Scale)
    {
        if (Range.X == 0 && Range.Y == 0)
        {
            return;
        }

        Hit.Damage.FindOrAdd(DamageType) += RollRange(Range) * Scale;
    };

    if (bIsAttack)
    {
        const float WeaponEffectiveness = Ability.WeaponDamageEffectiveness / 100.f;
        for (const TPair<EDamageType, FIntPoint>& Entry : Attacker.WeaponDamage)
        {
            AddRolledDamage(Entry.Key, Entry.Value, WeaponEffectiveness);
        }
    }

    for (const TPair<EDamageType, FIntPoint>& Entry : Ability.BaseDamage)
    {
        AddRolledDamage(Entry.Key, Entry.Value, 1.f);
    }

    const float AttributeMultiplier = 1.f
        + GetAttributeValue(Attacker.Attributes, Ability.ScalingAttribute) * Ability.DamagePerAttributePoint / 100.f;

    const float CritChance = bIsAttack ? Attacker.CritChance : Ability.CritChance;
    const float CritMultiplier = bIsAttack ? Attacker.CritMultiplier : Ability.CritMultiplier;
    Hit.bIsCritical = FMath::FRand() * 100.f < CritChance;

    const float TotalMultiplier = AttributeMultiplier * (Hit.bIsCritical ? CritMultiplier : 1.f);
    for (TPair<EDamageType, float>& Entry : Hit.Damage)
    {
        Entry.Value *= TotalMultiplier;
    }

    return Hit;
}

FCombatHitResult CombatMath::ResolveHit(const FCombatHit& Hit, const FCombatDefense& Defense, float& InOutHealth, float& InOutOvershield)
{
    FCombatHitResult Result;
    Result.bResolved = true;
    Result.bIsCritical = Hit.bIsCritical;

    if (Hit.bCanBeEvaded)
    {
        const float EvadeChance = FMath::Min(static_cast<float>(RollRange(Defense.EvadeChance)), MaxEvadeChance);
        if (FMath::FRand() * 100.f < EvadeChance)
        {
            Result.bEvaded = true;
            return Result;
        }
    }

    float TotalDamage = 0.f;
    for (const TPair<EDamageType, float>& Entry : Hit.Damage)
    {
        float DamageMultiplier = 1.f;
        if (Entry.Key == EDamageType::Physical)
        {
            const float Mitigation = static_cast<float>(FMath::Max(0, RollRange(Defense.PhysicalMitigation)));
            DamageMultiplier = 1.f - FMath::Min(Mitigation / (Mitigation + PhysicalMitigationScaling), MaxPhysicalReduction);
        }
        else
        {
            const float Resistance = FMath::Min(Defense.Resistances.FindRef(Entry.Key) - Hit.ResistancePenetration, MaxResistance);
            DamageMultiplier = 1.f - Resistance / 100.f;
        }

        const float DamageTaken = Entry.Value * DamageMultiplier;
        Result.DamageTaken.Add(Entry.Key, DamageTaken);
        TotalDamage += DamageTaken;
    }

    Result.OvershieldDamage = FMath::Min(InOutOvershield, TotalDamage);
    InOutOvershield -= Result.OvershieldDamage;

    Result.HealthDamage = FMath::Min(InOutHealth, TotalDamage - Result.OvershieldDamage);
    InOutHealth -= Result.HealthDamage;

    Result.bKilledTarget = InOutHealth <= 0.f;
    return Result;
}
