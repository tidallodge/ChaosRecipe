// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "CombatTypes.h"
#include "EnemyStruct.h"
#include "BattleManager.generated.h"

class UCoreMenu;
class UPlayerMinion;
class UEnemy;

// Broadcast once when a battle finishes. bMinionsWon is false for a loss or a timeout.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBattleEndedEvent, bool, bMinionsWon, float, BattleTime);

UCLASS()
class CHAOSRECIPE_API UBattleManager : public UObject
{
	GENERATED_BODY()

public:
	// Starts a battle whenever CoreMenu's BattlePlayButton is clicked, and shows the battle time there.
	UFUNCTION()
	void BindToCoreMenuEvents(UCoreMenu* CoreMenu);

	// Who fights in every battle from now on. Takes effect at the next StartBattle.
	void SetCombatants(const TArray<UPlayerMinion*>& InMinions, const TArray<UEnemy*>& InEnemies);

	// Restores every combatant to full, resets the battle timer to 0 and starts simulating. Ignored
	// while a battle is already running.
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StartBattle();

	// Stops the current battle where it is, without a winner (OnBattleEndedEvent isn't broadcast).
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StopBattle();

	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsBattleRunning() const { return bBattleRunning; }

	// Seconds since the current (or last) battle started.
	UFUNCTION(BlueprintPure, Category = "Battle")
	float GetBattleTime() const { return BattleTime; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBattleEndedEvent OnBattleEndedEvent;

protected:
	// One combatant's state within the running battle.
	struct FBattleCombatant
	{
		ICombatant* Combatant = nullptr;
		bool bIsMinion = false;

		// Battle time at which this combatant's cooldown is up and it can act again.
		float NextActionTime = 0.f;

		// Kept until it's defeated, then a new one is picked.
		ICombatant* Target = nullptr;
	};

	// An enemy killed during the current battle and what it dropped, shown once the battle ends.
	struct FBattleKill
	{
		FString EnemyName;
		FEnemyLoot Loot;
	};

	UFUNCTION()
	void HandleBattlePlayButtonClicked();

	// Bound to every enemy's OnKilledEvent while a battle is running, to record its drops in BattleKills.
	UFUNCTION()
	void HandleEnemyKilled(FString EnemyId, const FEnemyLoot& Loot);

	// "<EnemyName> Slain: +<Gold> Gold, +<Count> <CurrencyName>, ..." with one line per entry in BattleKills.
	FString BuildBattleKillsMessage() const;

	// Runs every BattleTickInterval while a battle is going: advances BattleTime, lets every combatant whose
	// cooldown is up act (in the order they became ready), and ends the battle once a side is defeated.
	void TickBattle();

	// The living combatant whose cooldown came up earliest at or before BattleTime, or null if none are ready.
	FBattleCombatant* FindNextReadyCombatant();

	// Picks Actor's target and an ability, uses it, then puts Actor on that ability's cooldown.
	void TakeAction(FBattleCombatant& Actor);

	// A random living combatant from the other side, or null if they're all defeated.
	ICombatant* PickTarget(const FBattleCombatant& Actor) const;

	bool IsSideDefeated(bool bMinionSide) const;

	// Ends the battle if either side is fully defeated. Returns true if it did.
	bool TryEndBattle();

	void EndBattle(bool bMinionsWon);

	UPROPERTY()
	TArray<TObjectPtr<UPlayerMinion>> Minions;

	UPROPERTY()
	TArray<TObjectPtr<UEnemy>> Enemies;

	// Rebuilt from Minions and Enemies at the start of each battle (minions first, so they win ties).
	TArray<FBattleCombatant> Combatants;

	// Cleared at the start of each battle.
	TArray<FBattleKill> BattleKills;

	UPROPERTY()
	TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;

	FTimerHandle BattleTickHandle;

	bool bBattleRunning = false;
	float BattleTime = 0.f;
	double BattleStartWorldTime = 0.0;
};
