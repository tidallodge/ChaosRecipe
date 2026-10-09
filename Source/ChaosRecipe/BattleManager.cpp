// BattleManager
/** Purpose: Simulate a battle between the player's minions and a group of enemies
    Every combatant picks a target and an attack/spell, hits with it, then waits out that ability's
    cooldown (one over its attack or cast rate) before acting again, until one side is defeated
*/

#include "BattleManager.h"
#include "CoreMenu.h"
#include "PlayerMinion.h"
#include "Enemy.h"
#include "CurrencyManager.h"
#include "Engine/World.h"

namespace
{
	// How often the battle advances. Actions still happen at their exact cooldown times (see
	// TickBattle); this only sets how quickly they're noticed and how often the timer display updates.
	constexpr float BattleTickInterval = 0.05f;

	// A battle where neither side can finish the other off (e.g. an enemy with no Damage) ends as a loss here.
	constexpr float MaxBattleDuration = 300.f;
}

void UBattleManager::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{
	if (!CoreMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("no CoreMenu for BattleManager"));
		return;
	}

	BoundCoreMenu = CoreMenu;
	CoreMenu->OnBattlePlayButtonClickedEvent.AddDynamic(this, &UBattleManager::HandleBattlePlayButtonClicked);
}

void UBattleManager::SetCombatants(const TArray<UPlayerMinion*>& InMinions, const TArray<UEnemy*>& InEnemies)
{
	Minions.Reset();
	for (UPlayerMinion* Minion : InMinions)
	{
		if (Minion)
		{
			Minions.Add(Minion);
		}
	}

	Enemies.Reset();
	for (UEnemy* Enemy : InEnemies)
	{
		if (Enemy)
		{
			Enemies.Add(Enemy);
		}
	}
}

void UBattleManager::HandleBattlePlayButtonClicked()
{
	StartBattle();
}

void UBattleManager::StartBattle()
{
	if (bBattleRunning)
	{
		if (BoundCoreMenu)
		{
			BoundCoreMenu->SetWarningText(TEXT("A battle is already running."));
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: No world to run a battle in."));
		return;
	}

	// Every battle starts fresh, so damage taken in the last one doesn't carry over.
	Combatants.Reset();
	for (UPlayerMinion* Minion : Minions)
	{
		Minion->RestoreToFull();
		Combatants.AddDefaulted_GetRef().Combatant = Minion;
		Combatants.Last().bIsMinion = true;
	}
	for (UEnemy* Enemy : Enemies)
	{
		Enemy->RestoreToFull();
		Combatants.AddDefaulted_GetRef().Combatant = Enemy;
	}

	if (IsSideDefeated(true) || IsSideDefeated(false))
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: Need at least one minion and one enemy able to fight (minions=%d, enemies=%d)."),
			Minions.Num(), Enemies.Num());
		if (BoundCoreMenu)
		{
			BoundCoreMenu->SetWarningText(TEXT("Need a minion and an enemy to battle."));
		}
		return;
	}

	BattleKills.Reset();
	for (UEnemy* Enemy : Enemies)
	{
		Enemy->OnKilledEvent.AddUniqueDynamic(this, &UBattleManager::HandleEnemyKilled);
	}

	bBattleRunning = true;
	BattleTime = 0.f;
	BattleStartWorldTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(BattleTickHandle, this, &UBattleManager::TickBattle, BattleTickInterval, true);

	if (BoundCoreMenu)
	{
		BoundCoreMenu->SetBattleTimerText(BattleTime);
		BoundCoreMenu->LogToScreen(FString::Printf(TEXT("Battle started: %d minion(s) vs %d enemy(s)."), Minions.Num(), Enemies.Num()));
	}
}

void UBattleManager::StopBattle()
{
	bBattleRunning = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BattleTickHandle);
	}

	for (UEnemy* Enemy : Enemies)
	{
		Enemy->OnKilledEvent.RemoveDynamic(this, &UBattleManager::HandleEnemyKilled);
	}
}

void UBattleManager::HandleEnemyKilled(FString EnemyId, const FEnemyLoot& Loot)
{
	FString EnemyName = EnemyId;
	for (UEnemy* Enemy : Enemies)
	{
		if (Enemy->GetEnemyId() == EnemyId)
		{
			EnemyName = Enemy->GetEnemyName().ToString();
			break;
		}
	}

	BattleKills.Add({ EnemyName, Loot });
}

FString UBattleManager::BuildBattleKillsMessage() const
{
	UCurrencyManager CurrencyManager;
	TArray<FString> Lines;
	for (const FBattleKill& Kill : BattleKills)
	{
		TArray<FString> Drops;
		if (Kill.Loot.Gold > 0)
		{
			Drops.Add(FString::Printf(TEXT("+%d Gold"), Kill.Loot.Gold));
		}

		for (const TPair<FString, int32>& Entry : Kill.Loot.CurrencyCounts)
		{
			FCurrencyStruct Currency;
			const FString CurrencyName = CurrencyManager.LoadCurrencyDataRow(Entry.Key, Currency)
				? Currency.CurrencyName.ToString()
				: Entry.Key;
			Drops.Add(FString::Printf(TEXT("+%d %s"), Entry.Value, *CurrencyName));
		}

		FString Line = FString::Printf(TEXT("%s Slain"), *Kill.EnemyName);
		if (Drops.Num() > 0)
		{
			Line += TEXT(": ") + FString::Join(Drops, TEXT(", "));
		}
		Lines.Add(Line);
	}

	return FString::Join(Lines, TEXT("\n"));
}

void UBattleManager::TickBattle()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		StopBattle();
		return;
	}

	// Measured from the world clock rather than counted in ticks, so a late or skipped tick can't make
	// the battle (or its timer) drift.
	BattleTime = static_cast<float>(World->GetTimeSeconds() - BattleStartWorldTime);
	if (BoundCoreMenu)
	{
		BoundCoreMenu->SetBattleTimerText(BattleTime);
	}

	// Resolve every action that came due since the last tick in the order it came due, so a combatant
	// killed earlier in this tick doesn't still get its turn.
	while (FBattleCombatant* Actor = FindNextReadyCombatant())
	{
		TakeAction(*Actor);
		if (TryEndBattle())
		{
			return;
		}
	}

	if (BattleTime >= MaxBattleDuration)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: Battle timed out after %.0fs."), MaxBattleDuration);
		EndBattle(false);
	}
}

UBattleManager::FBattleCombatant* UBattleManager::FindNextReadyCombatant()
{
	FBattleCombatant* NextActor = nullptr;
	for (FBattleCombatant& Entry : Combatants)
	{
		if (Entry.Combatant->IsDefeated() || Entry.NextActionTime > BattleTime)
		{
			continue;
		}

		// Strictly earlier, so ties go to whoever is first in Combatants (minions).
		if (!NextActor || Entry.NextActionTime < NextActor->NextActionTime)
		{
			NextActor = &Entry;
		}
	}
	return NextActor;
}

void UBattleManager::TakeAction(FBattleCombatant& Actor)
{
	if (!Actor.Target || Actor.Target->IsDefeated())
	{
		Actor.Target = PickTarget(Actor);
	}

	TArray<FCombatAbilityStruct> Abilities = Actor.Combatant->GetBattleAbilities();
	if (Abilities.Num() == 0)
	{
		Abilities.Add(CombatMath::MakeBasicAttack());
	}
	const FCombatAbilityStruct& Ability = Abilities[FMath::RandRange(0, Abilities.Num() - 1)];

	// Added to when this combatant was due rather than to the current tick, so its attack/cast rate holds
	// exactly however the ticks happen to line up.
	Actor.NextActionTime += CombatMath::GetAbilityCooldown(Ability, Actor.Combatant->GetBattleAttackRate());

	if (!Actor.Target)
	{
		return;
	}

	const FCombatHitResult Result = Actor.Combatant->UseAbility(Ability, Actor.Target);

	UE_LOG(LogTemp, Warning, TEXT("BattleManager: [%.2fs] %s used %s on %s: %.1f damage%s%s (next action at %.2fs)"),
		BattleTime, *Actor.Combatant->GetCombatantName(), *Ability.AbilityName.ToString(), *Actor.Target->GetCombatantName(),
		Result.HealthDamage + Result.OvershieldDamage, Result.bIsCritical ? TEXT(", critical") : TEXT(""),
		Result.bEvaded ? TEXT(", evaded") : TEXT(""), Actor.NextActionTime);
}

ICombatant* UBattleManager::PickTarget(const FBattleCombatant& Actor) const
{
	TArray<ICombatant*> Candidates;
	for (const FBattleCombatant& Entry : Combatants)
	{
		if (Entry.bIsMinion != Actor.bIsMinion && !Entry.Combatant->IsDefeated())
		{
			Candidates.Add(Entry.Combatant);
		}
	}

	return Candidates.Num() > 0 ? Candidates[FMath::RandRange(0, Candidates.Num() - 1)] : nullptr;
}

bool UBattleManager::IsSideDefeated(bool bMinionSide) const
{
	for (const FBattleCombatant& Entry : Combatants)
	{
		if (Entry.bIsMinion == bMinionSide && !Entry.Combatant->IsDefeated())
		{
			return false;
		}
	}
	return true;
}

bool UBattleManager::TryEndBattle()
{
	if (IsSideDefeated(false))
	{
		EndBattle(true);
		return true;
	}

	if (IsSideDefeated(true))
	{
		EndBattle(false);
		return true;
	}

	return false;
}

void UBattleManager::EndBattle(bool bMinionsWon)
{
	StopBattle();

	if (BoundCoreMenu)
	{
		BoundCoreMenu->SetBattleTimerText(BattleTime);
		BoundCoreMenu->LogToScreen(FString::Printf(TEXT("Battle %s in %.1fs."), bMinionsWon ? TEXT("won") : TEXT("lost"), BattleTime));

		if (BattleKills.Num() > 0)
		{
			BoundCoreMenu->SetWarningText(BuildBattleKillsMessage());
		}
	}

	OnBattleEndedEvent.Broadcast(bMinionsWon, BattleTime);
}
