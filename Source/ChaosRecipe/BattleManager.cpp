// BattleManager
/** Purpose: Simulate a battle between the player's minions and a zone's enemies, stage by stage
    Every stage spawns a random enemy group from Zone_DT after a short pause, and the party carries its health from one
    stage to the next
    Clearing a stage grants its Zone_DT StageEndDrops (if it has any) before the battle moves on
    Minions regenerate life every second of the battle, and recover more after each cleared stage
    Every combatant picks a target and an attack/spell, hits with it, then waits out that ability's
    cooldown (one over its attack or cast rate) before acting again, until one side is defeated
*/

#include "BattleManager.h"
#include "CoreMenu.h"
#include "PlayerMinion.h"
#include "MinionHandler.h"
#include "PlayerInventory.h"
#include "Enemy.h"
#include "BattleScene.h"
#include "CurrencyManager.h"
#include "DataTableHelpers.h"
#include "Engine/World.h"

namespace
{
	// How often the battle advances. Actions still happen at their exact cooldown times (see
	// TickBattle); this only sets how quickly they're noticed and how often the timer display updates.
	constexpr float BattleTickInterval = 0.05f;

	// A stage where neither side can finish the other off (e.g. an enemy with no Damage) ends the battle as a loss here.
	constexpr float MaxStageDuration = 300.f;

	// The pause before each stage's enemies spawn, the first stage's included. The battle clock keeps running through it.
	constexpr float StageSpawnDelay = 1.f;

	// The pause between a stage's enemies spawning and anyone acting, so their meshes are on screen before the fight
	// starts. The battle clock keeps running through it too.
	constexpr float StageFightDelay = 1.f;

	// How often minions regenerate their LifeRegenPerSecond, in battle time.
	constexpr float LifeRegenInterval = 1.f;

	// The zone's ZoneName, or its ZoneId if it has none.
	FString GetZoneDisplayName(const FZoneStruct& Zone)
	{
		return (Zone.ZoneName.IsEmpty() ? Zone.ZoneId : Zone.ZoneName).ToString();
	}

	// Up to one decimal place, so whole amounts read as "17" and scaled ones as "17.5".
	FString FormatDamage(float Damage)
	{
		static const FNumberFormattingOptions Options = FNumberFormattingOptions().SetUseGrouping(false).SetMaximumFractionalDigits(1);
		return FText::AsNumber(Damage, &Options).ToString();
	}

	// "<Attacker> <Ability> -> <Target>: Physical rolled 17 (13-21), dealt 15.2 | Fire rolled 4 (3-5), dealt 4 | Total 19.2",
	// with an entry per damage type: its roll, the range it was rolled from, and the damage that got through the target's
	// defenses (after attribute scaling, crit and mitigation/resistance).
	FString BuildHitMessage(const FString& AttackerName, const FString& AbilityName, const FString& TargetName, const FCombatHitResult& Result)
	{
		TArray<FString> Entries;
		float TotalDamage = 0.f;
		for (const TPair<EDamageType, FCombatDamageRoll>& Entry : Result.Rolls)
		{
			FString Text = FString::Printf(TEXT("%s rolled %s (%s-%s)"), *UEnum::GetDisplayValueAsText(Entry.Key).ToString(),
				*FormatDamage(Entry.Value.Roll), *FormatDamage(Entry.Value.Min), *FormatDamage(Entry.Value.Max));
			if (!Result.bEvaded)
			{
				const float DamageTaken = Result.DamageTaken.FindRef(Entry.Key);
				Text += FString::Printf(TEXT(", dealt %s"), *FormatDamage(DamageTaken));
				TotalDamage += DamageTaken;
			}
			Entries.Add(Text);
		}

		FString Message = FString::Printf(TEXT("%s %s -> %s: %s"), *AttackerName, *AbilityName, *TargetName,
			Entries.Num() > 0 ? *FString::Join(Entries, TEXT(" | ")) : TEXT("no damage"));
		if (Result.bEvaded)
		{
			return Message + TEXT(" | Evaded");
		}

		Message += FString::Printf(TEXT(" | Total %s"), *FormatDamage(TotalDamage));
		if (Result.bIsCritical)
		{
			Message += TEXT(" (critical)");
		}
		return Message;
	}
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

void UBattleManager::BindToMinionHandler(UMinionHandler* MinionHandler)
{
	if (!MinionHandler)
	{
		UE_LOG(LogTemp, Error, TEXT("no MinionHandler for BattleManager"));
		return;
	}

	BoundMinionHandler = MinionHandler;
}

void UBattleManager::BindToPlayerInventory(UPlayerInventory* PlayerInventory)
{
	if (!PlayerInventory)
	{
		UE_LOG(LogTemp, Error, TEXT("no PlayerInventory for BattleManager"));
		return;
	}

	BoundPlayerInventory = PlayerInventory;
}

void UBattleManager::BindToBattleScene(ABattleScene* BattleScene)
{
	if (!BattleScene)
	{
		UE_LOG(LogTemp, Error, TEXT("no BattleScene for BattleManager"));
		return;
	}

	BoundBattleScene = BattleScene;
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

	const FString ZoneId = BoundCoreMenu ? BoundCoreMenu->GetActiveZoneId() : FString();
	if (ZoneId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: No zone selected to battle in."));
		if (BoundCoreMenu)
		{
			BoundCoreMenu->SetWarningText(TEXT("Pick a zone on the map first."));
		}
		return;
	}

	FZoneStruct LoadedZone;
	const bool bFoundZone = DataTableHelpers::FindRow(ZoneDataTablePath,
		[&ZoneId](const FZoneStruct& Row) { return Row.ZoneId.ToString().Equals(ZoneId, ESearchCase::IgnoreCase); }, LoadedZone);
	if (!bFoundZone || LoadedZone.GetStageCount() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: Zone '%s' isn't in Zone_DT or has no stages."), *ZoneId);
		if (BoundCoreMenu)
		{
			BoundCoreMenu->SetWarningText(TEXT("That zone has no stages to battle through."));
		}
		return;
	}

	// Read now rather than kept up to date, since minions can join or leave the active party between battles.
	const TArray<UPlayerMinion*> ActivePartyMinions = BoundMinionHandler ? BoundMinionHandler->GetActivePartyMinions() : TArray<UPlayerMinion*>();
	if (ActivePartyMinions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: No minions in the active party to battle with."));
		if (BoundCoreMenu)
		{
			BoundCoreMenu->SetWarningText(TEXT("Need a minion in your active party to battle."));
		}
		return;
	}

	LoadedZone.RemoveUnknownCurrencyDrops();
	Zone = LoadedZone;
	Minions.Reset();
	Minions.Append(ActivePartyMinions);

	// The party starts fresh, then carries whatever damage it takes from stage to stage until the battle ends.
	for (UPlayerMinion* Minion : Minions)
	{
		Minion->RestoreToFull();
	}

	BattleTime = 0.f;
	NextLifeRegenTime = LifeRegenInterval;
	QueueStage(0);

	bBattleRunning = true;
	BattleStartWorldTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(BattleTickHandle, this, &UBattleManager::TickBattle, BattleTickInterval, true);

	if (BoundCoreMenu)
	{
		// The last battle's enemies go now, rather than staying up through the pause before the first stage spawns.
		BoundCoreMenu->SetBattleEnemies(TArray<UEnemy*>());
		BoundCoreMenu->SetBattleTimerText(BattleTime);
		BoundCoreMenu->SetWarningText(FString::Printf(TEXT("Entering %s: %d stage(s)."), *GetZoneDisplayName(Zone), Zone.GetStageCount()));
	}
}

bool UBattleManager::StartStage(int32 InStageIndex)
{
	StageIndex = InStageIndex;

	// The last stage's enemies are all dead by now (or from an earlier battle), so they're let go.
	for (UEnemy* Enemy : Enemies)
	{
		Enemy->OnKilledEvent.RemoveDynamic(this, &UBattleManager::HandleEnemyKilled);
	}
	Enemies.Reset();

	TArray<UEnemy*> SpawnedEnemies;
	for (const FString& EnemyId : Zone.RollEnemyGroup(StageIndex))
	{
		UEnemy* Enemy = NewObject<UEnemy>(this);
		if (!Enemy->InitializeEnemy(EnemyId))
		{
			continue;
		}

		Enemy->OnKilledEvent.AddDynamic(this, &UBattleManager::HandleEnemyKilled);
		if (BoundPlayerInventory)
		{
			BoundPlayerInventory->BindToEnemyEvents(Enemy);
		}
		SpawnedEnemies.Add(Enemy);
	}
	Enemies.Append(SpawnedEnemies);

	if (BoundCoreMenu)
	{
		BoundCoreMenu->SetBattleEnemies(SpawnedEnemies);
	}
	if (BoundBattleScene)
	{
		BoundBattleScene->ShowEnemies(SpawnedEnemies);
	}

	if (Enemies.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: %s spawned no enemies."), *GetStageLabel());
		return false;
	}

	BattleKills.Reset();
	StageEndLoot = FEnemyLoot();
	StageStartTime = BattleTime;

	// Everyone is ready to act together once StageFightDelay has passed, so nothing happens until the new enemies are
	// on screen. Minions killed in an earlier stage stay down: they're listed, but never act or get targeted.
	const float FirstActionTime = BattleTime + StageFightDelay;
	Combatants.Reset();
	for (UPlayerMinion* Minion : Minions)
	{
		FBattleCombatant& Entry = Combatants.AddDefaulted_GetRef();
		Entry.Combatant = Minion;
		Entry.bIsMinion = true;
		Entry.NextActionTime = FirstActionTime;
	}
	for (UEnemy* Enemy : Enemies)
	{
		FBattleCombatant& Entry = Combatants.AddDefaulted_GetRef();
		Entry.Combatant = Enemy;
		Entry.NextActionTime = FirstActionTime;
	}

	UE_LOG(LogTemp, Warning, TEXT("BattleManager: [%.2fs] %s started: %d minion(s) vs %d enemy(s), fighting from %.2fs."),
		BattleTime, *GetStageLabel(), Minions.Num(), Enemies.Num(), FirstActionTime);
	return true;
}

void UBattleManager::QueueStage(int32 InStageIndex)
{
	PendingStageIndex = InStageIndex;
	PendingStageStartTime = BattleTime + StageSpawnDelay;
}

void UBattleManager::StopBattle()
{
	bBattleRunning = false;
	PendingStageIndex = INDEX_NONE;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BattleTickHandle);
	}

	for (UEnemy* Enemy : Enemies)
	{
		Enemy->OnKilledEvent.RemoveDynamic(this, &UBattleManager::HandleEnemyKilled);
	}

	if (BoundBattleScene)
	{
		BoundBattleScene->ClearEnemies();
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

FString UBattleManager::BuildStageDropsMessage(const FString& Headline) const
{
	UCurrencyManager CurrencyManager;

	// "+<Gold> Gold, +<Count> <CurrencyName>, ...", or empty if Loot has nothing in it.
	auto FormatLoot = [&CurrencyManager](const FEnemyLoot& Loot)
	{
		TArray<FString> Drops;
		if (Loot.Gold > 0)
		{
			Drops.Add(FString::Printf(TEXT("+%d Gold"), Loot.Gold));
		}

		for (const TPair<FString, int32>& Entry : Loot.CurrencyCounts)
		{
			FCurrencyStruct Currency;
			const FString CurrencyName = CurrencyManager.LoadCurrencyDataRow(Entry.Key, Currency)
				? Currency.CurrencyName.ToString()
				: Entry.Key;
			Drops.Add(FString::Printf(TEXT("+%d %s"), Entry.Value, *CurrencyName));
		}
		return FString::Join(Drops, TEXT(", "));
	};

	TArray<FString> Lines;
	for (const FBattleKill& Kill : BattleKills)
	{
		FString Line = FString::Printf(TEXT("%s Slain"), *Kill.EnemyName);
		const FString Drops = FormatLoot(Kill.Loot);
		if (!Drops.IsEmpty())
		{
			Line += TEXT(": ") + Drops;
		}
		Lines.Add(Line);
	}

	if (!StageEndLoot.IsEmpty())
	{
		Lines.Add(TEXT("Stage End Drops: ") + FormatLoot(StageEndLoot));
	}

	Lines.Add(Headline);
	return FString::Join(Lines, TEXT("\n"));
}

void UBattleManager::RollStageEndDrops()
{
	StageEndLoot = Zone.RollStageEndDrops(StageIndex);
	if (StageEndLoot.IsEmpty())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("BattleManager: [%.2fs] %s end drops: %d gold and %d currency type(s)."),
		BattleTime, *GetStageLabel(), StageEndLoot.Gold, StageEndLoot.CurrencyCounts.Num());
	if (BoundPlayerInventory)
	{
		BoundPlayerInventory->GrantLoot(GetStageLabel(), StageEndLoot);
	}
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

	// Only life regen happens while a stage waits out its pause; once it's over, the stage's enemies spawn and the stage
	// starts.
	if (PendingStageIndex != INDEX_NONE)
	{
		if (BattleTime < PendingStageStartTime)
		{
			ApplyLifeRegen(BattleTime);
			return;
		}

		const int32 NextStageIndex = PendingStageIndex;
		PendingStageIndex = INDEX_NONE;
		if (!StartStage(NextStageIndex))
		{
			EndBattle(false);
			if (BoundCoreMenu)
			{
				BoundCoreMenu->SetWarningText(FString::Printf(TEXT("%s has no enemies to battle."), *GetStageLabel()));
			}
			return;
		}
	}

	// Resolve every action that came due since the last tick in the order it came due, so a combatant
	// killed earlier in this tick doesn't still get its turn. Life regen due before each action goes first, so a
	// minion healed in time survives a hit it otherwise wouldn't.
	while (true)
	{
		FBattleCombatant* Actor = FindNextReadyCombatant();
		ApplyLifeRegen(Actor ? Actor->NextActionTime : BattleTime);
		if (!Actor)
		{
			break;
		}

		TakeAction(*Actor);
		if (TryEndStage())
		{
			return;
		}
	}

	if (BattleTime - StageStartTime >= MaxStageDuration)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: %s timed out after %.0fs."), *GetStageLabel(), MaxStageDuration);
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

	if (Result.bResolved)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: [%.2fs] %s"), BattleTime, *BuildHitMessage(Actor.Combatant->GetCombatantName(),
			Ability.AbilityName.ToString(), Actor.Target->GetCombatantName(), Result));
	}
}

void UBattleManager::ApplyLifeRegen(float UpToTime)
{
	while (NextLifeRegenTime <= UpToTime)
	{
		for (UPlayerMinion* Minion : Minions)
		{
			const float Healed = Minion->RecoverHealth(static_cast<float>(Minion->GetLifeRegenPerSecond()));
			if (Healed > 0.f)
			{
				UE_LOG(LogTemp, Warning, TEXT("BattleManager: [%.2fs] %s regenerated %.0f life (Health=%.1f/%.0f)"),
					NextLifeRegenTime, *Minion->GetCombatantName(), Healed, Minion->GetCurrentHealth(), Minion->GetMaxHealth());
			}
		}
		NextLifeRegenTime += LifeRegenInterval;
	}
}

void UBattleManager::ApplyStageLifeRecovery()
{
	for (UPlayerMinion* Minion : Minions)
	{
		const float Healed = Minion->RecoverHealth(static_cast<float>(Minion->GetLifeRecoveryPerStage()));
		if (Healed > 0.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("BattleManager: [%.2fs] %s recovered %.0f life after %s (Health=%.1f/%.0f)"),
				BattleTime, *Minion->GetCombatantName(), Healed, *GetStageLabel(), Minion->GetCurrentHealth(), Minion->GetMaxHealth());
		}
	}
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

bool UBattleManager::TryEndStage()
{
	if (IsSideDefeated(false))
	{
		// Before anything else, so the stage's end drops are granted (and shown) whether the battle moves on or is won
		// here.
		RollStageEndDrops();

		if (StageIndex + 1 >= Zone.GetStageCount())
		{
			EndBattle(true);
			return true;
		}

		// Said now, since the next stage starts counting its own kills and drops.
		const FString ClearedMessage = FString::Printf(TEXT("%s cleared!"), *GetStageLabel());
		if (BoundCoreMenu)
		{
			BoundCoreMenu->LogToScreen(ClearedMessage);
			BoundCoreMenu->SetWarningText(BuildStageDropsMessage(ClearedMessage));
		}

		// On to the next stage after its pause, with the party as it is once it's recovered.
		ApplyStageLifeRecovery();
		QueueStage(StageIndex + 1);
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

	// The damage the party carried from stage to stage is healed only now that the battle's over.
	for (UPlayerMinion* Minion : Minions)
	{
		Minion->RestoreToFull();
	}

	if (BoundCoreMenu)
	{
		const FString Outcome = bMinionsWon
			? FString::Printf(TEXT("%s cleared in %.1fs!"), *GetZoneDisplayName(Zone), BattleTime)
			: FString::Printf(TEXT("Battle lost on %s after %.1fs."), *GetStageLabel(), BattleTime);
		BoundCoreMenu->SetBattleTimerText(BattleTime);
		BoundCoreMenu->LogToScreen(Outcome);
		BoundCoreMenu->SetWarningText(BuildStageDropsMessage(Outcome));
	}

	OnBattleEndedEvent.Broadcast(bMinionsWon, BattleTime);
}

FString UBattleManager::GetStageLabel() const
{
	return FString::Printf(TEXT("%s stage %d/%d"), *GetZoneDisplayName(Zone), StageIndex + 1, Zone.GetStageCount());
}
