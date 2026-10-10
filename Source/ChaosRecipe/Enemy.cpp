// Enemy
/** Purpose: A basic combatant built from an Enemy_DT row
    Attacks and casts spells at anything that implements ICombatTarget, and takes hits through it too
    Rolls its gold and currency drops when killed
*/

#include "Enemy.h"
#include "CurrencyManager.h"
#include "DataTableHelpers.h"

namespace
{
	const TCHAR* EnemyDataTablePath = TEXT("/Game/EnemyData/Enemy_DT.Enemy_DT");
}

bool UEnemy::InitializeEnemy(const FString& InEnemyId)
{
	FEnemyStruct LoadedEnemyData;
	const bool bFoundEnemy = DataTableHelpers::FindRow(EnemyDataTablePath,
		[&InEnemyId](const FEnemyStruct& Row) { return Row.EnemyId.ToString().Equals(InEnemyId, ESearchCase::IgnoreCase); }, LoadedEnemyData);
	if (!bFoundEnemy)
	{
		UE_LOG(LogTemp, Error, TEXT("Enemy: No Enemy_DT row for '%s'."), *InEnemyId);
		return false;
	}

	EnemyData = LoadedEnemyData;

	// Drop any CurrencyId that isn't in Currency_DT, since PlayerInventory would otherwise happily grant
	// a stack of a currency that doesn't exist.
	UCurrencyManager CurrencyManager;
	EnemyData.CurrencyDrops.RemoveAll([&CurrencyManager, &InEnemyId](const FEnemyCurrencyDrop& Drop)
	{
		FCurrencyStruct Currency;
		if (CurrencyManager.LoadCurrencyDataRow(Drop.CurrencyId, Currency))
		{
			return false;
		}

		UE_LOG(LogTemp, Warning, TEXT("Enemy: %s drops unknown currency '%s'; ignoring that drop."), *InEnemyId, *Drop.CurrencyId);
		return true;
	});

	Offense = FCombatOffense();
	Offense.WeaponDamage = EnemyData.Damage;
	Offense.AttackRate = EnemyData.AttackRate;

	Defense = FCombatDefense();
	Defense.PhysicalMitigation = FIntPoint(EnemyData.PhysicalMitigation, EnemyData.PhysicalMitigation);
	Defense.EvadeChance = FIntPoint(EnemyData.EvadeChance, EnemyData.EvadeChance);
	Defense.Overshield = FIntPoint(EnemyData.Overshield, EnemyData.Overshield);
	Defense.Resistances = EnemyData.Resistances;

	RestoreToFull();

	UE_LOG(LogTemp, Warning, TEXT("Enemy: Initialized %s (Health=%d, Overshield=%d, Spells=%d)"),
		*EnemyData.EnemyName.ToString(), EnemyData.MaxHealth, EnemyData.Overshield, EnemyData.Spells.Num());
	return true;
}

void UEnemy::RestoreToFull()
{
	CurrentHealth = GetMaxHealth();
	CurrentOvershield = static_cast<float>(EnemyData.Overshield);
	BroadcastHealthChanged();
}

FCombatHitResult UEnemy::Attack(ICombatTarget* Target)
{
	// An enemy's "weapon damage" is its own Damage (see InitializeEnemy).
	return UseAbility(CombatMath::MakeBasicAttack(), Target);
}

TArray<FCombatAbilityStruct> UEnemy::GetBattleAbilities() const
{
	TArray<FCombatAbilityStruct> Abilities = { CombatMath::MakeBasicAttack() };
	Abilities.Append(EnemyData.Spells);
	return Abilities;
}

FCombatHitResult UEnemy::CastSpell(const FString& AbilityId, ICombatTarget* Target)
{
	const FCombatAbilityStruct* Spell = EnemyData.Spells.FindByPredicate(
		[&AbilityId](const FCombatAbilityStruct& Ability)
		{
			return Ability.AbilityId.ToString().Equals(AbilityId, ESearchCase::IgnoreCase);
		});
	if (!Spell)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy: %s has no spell '%s'."), *GetEnemyId(), *AbilityId);
		return FCombatHitResult();
	}

	return UseAbility(*Spell, Target);
}

FCombatHitResult UEnemy::UseAbility(const FCombatAbilityStruct& Ability, ICombatTarget* Target)
{
	if (IsDefeated() || !Target || Target->IsDefeated())
	{
		return FCombatHitResult();
	}

	const FCombatHit Hit = CombatMath::BuildHit(Ability, Offense);
	return Target->ApplyHit(Hit);
}

FCombatHitResult UEnemy::ApplyHit(const FCombatHit& Hit)
{
	if (IsDefeated())
	{
		return FCombatHitResult();
	}

	const FCombatHitResult Result = CombatMath::ResolveHit(Hit, Defense, CurrentHealth, CurrentOvershield);

	UE_LOG(LogTemp, Warning, TEXT("Enemy: %s hit for %.1f health / %.1f overshield%s%s (Health=%.1f/%.0f)"),
		*GetEnemyId(), Result.HealthDamage, Result.OvershieldDamage, Result.bIsCritical ? TEXT(", critical") : TEXT(""),
		Result.bEvaded ? TEXT(", evaded") : TEXT(""), CurrentHealth, GetMaxHealth());

	if (Result.HealthDamage > 0.f || Result.OvershieldDamage > 0.f)
	{
		BroadcastHealthChanged();
	}
	OnHitTakenEvent.Broadcast(Result);

	if (Result.bKilledTarget)
	{
		const FEnemyLoot Loot = RollLoot();
		UE_LOG(LogTemp, Warning, TEXT("Enemy: %s killed, dropped %d gold and %d currency type(s)."),
			*GetEnemyId(), Loot.Gold, Loot.CurrencyCounts.Num());
		OnKilledEvent.Broadcast(GetEnemyId(), Loot);
	}
	return Result;
}

bool UEnemy::IsDefeated() const
{
	return CurrentHealth <= 0.f;
}

FEnemyLoot UEnemy::RollLoot() const
{
	FEnemyLoot Loot;
	Loot.Gold = FMath::Max(0, CombatMath::RollRange(EnemyData.GoldDrop));

	for (const FEnemyCurrencyDrop& Drop : EnemyData.CurrencyDrops)
	{
		if (FMath::FRand() * 100.f >= Drop.DropChance)
		{
			continue;
		}

		const int32 Count = CombatMath::RollRange(Drop.DropCount);
		if (Count > 0)
		{
			Loot.CurrencyCounts.FindOrAdd(Drop.CurrencyId) += Count;
		}
	}

	return Loot;
}

TArray<FString> UEnemy::GetSpellIds() const
{
	TArray<FString> SpellIds;
	for (const FCombatAbilityStruct& Spell : EnemyData.Spells)
	{
		SpellIds.Add(Spell.AbilityId.ToString());
	}
	return SpellIds;
}

TArray<FString> UEnemy::GetCurrencyIdOptions()
{
	TArray<FString> CurrencyIds;
	TArray<FCurrencyStruct> Currencies;
	UCurrencyManager CurrencyManager;
	if (CurrencyManager.LoadAllCurrencyRows(Currencies))
	{
		for (const FCurrencyStruct& Currency : Currencies)
		{
			CurrencyIds.Add(Currency.CurrencyId.ToString());
		}
	}
	return CurrencyIds;
}

void UEnemy::BroadcastHealthChanged()
{
	OnHealthChangedEvent.Broadcast(CurrentHealth, GetMaxHealth(), CurrentOvershield);
}
