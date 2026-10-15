// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "CombatTypes.h"
#include "EnemyStruct.h"
#include "ZoneStruct.h"
#include "BattleManager.generated.h"

class UCoreMenu;
class UMinionHandler;
class UPlayerInventory;
class UPlayerMinion;
class UEnemy;
class ABattleScene;

// Broadcast once when a battle (a run through a zone's stages) finishes. bMinionsWon is true only if every stage was
// cleared; false for a loss or a timeout.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBattleEndedEvent, bool, bMinionsWon, float, BattleTime);

UCLASS()
class CHAOSRECIPE_API UBattleManager : public UObject
{
	GENERATED_BODY()

public:
	// Starts a battle in CoreMenu's active zone whenever its BattlePlayButton is clicked, and shows the battle time
	// and each stage's enemies there.
	UFUNCTION()
	void BindToCoreMenuEvents(UCoreMenu* CoreMenu);

	// Every battle from now on is fought by MinionHandler's active party, as it stands when the battle starts.
	UFUNCTION()
	void BindToMinionHandler(UMinionHandler* MinionHandler);

	// Lets every enemy spawned from now on grant the player its drops when it's killed.
	UFUNCTION()
	void BindToPlayerInventory(UPlayerInventory* PlayerInventory);

	// Shows each stage's enemies as their meshes in BattleScene, and clears them when the battle stops.
	UFUNCTION()
	void BindToBattleScene(ABattleScene* BattleScene);

	// Sends the active party through every stage of CoreMenu's active zone in order: each stage spawns a new random
	// enemy group (FZoneStruct::RollEnemyGroup) after a short pause, both at the start of the battle and after each stage
	// is cleared, and the fight starts a moment after it spawns. The party starts at full health and keeps whatever
	// damage it takes from stage to stage, until the battle ends: by clearing the last stage, losing every minion, or a
	// stage timing out. Ignored while a battle is already running.
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StartBattle();

	// Stops the current battle where it is, without a winner (OnBattleEndedEvent isn't broadcast), and clears its
	// enemies from the BattleScene.
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StopBattle();

	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsBattleRunning() const { return bBattleRunning; }

	// Seconds since the current (or last) battle started, across all of its stages.
	UFUNCTION(BlueprintPure, Category = "Battle")
	float GetBattleTime() const { return BattleTime; }

	// Which of the zone's stages the current (or last) battle is on, from 0.
	UFUNCTION(BlueprintPure, Category = "Battle")
	int32 GetStageIndex() const { return StageIndex; }

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

	// An enemy killed during the current stage and what it dropped, shown once the stage ends.
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

	// Spawns stage InStageIndex's enemy group from Zone (replacing the last stage's enemies), shows it in CoreMenu and
	// the BattleScene, and lines it up against the party as it is now, everyone ready to act together after
	// StageFightDelay. Returns false if no enemy could be spawned.
	bool StartStage(int32 InStageIndex);

	// Has stage InStageIndex start (StartStage) once StageSpawnDelay has passed, with nothing happening in the battle
	// until then.
	void QueueStage(int32 InStageIndex);

	// Runs every BattleTickInterval while a battle is going: advances BattleTime, starts a queued stage once its pause
	// is over, lets every combatant whose cooldown is up act (in the order they became ready), queues the next stage
	// once one is cleared, and ends the battle once the party is defeated, the last stage is cleared, or a stage takes
	// too long.
	void TickBattle();

	// The living combatant whose cooldown came up earliest at or before BattleTime, or null if none are ready.
	FBattleCombatant* FindNextReadyCombatant();

	// Picks Actor's target and an ability, uses it, then puts Actor on that ability's cooldown.
	void TakeAction(FBattleCombatant& Actor);

	// A random living combatant from the other side, or null if they're all defeated.
	ICombatant* PickTarget(const FBattleCombatant& Actor) const;

	bool IsSideDefeated(bool bMinionSide) const;

	// Once a side is fully defeated: the party losing ends the battle, and the enemies losing queues the next stage,
	// or ends the battle as won after the last one. Returns true if the stage is over (the battle ended, or the next
	// stage is queued).
	bool TryEndStage();

	// Stops the battle, puts the party back to full health, and says how it went.
	void EndBattle(bool bMinionsWon);

	// "<ZoneName> stage 2/5", for messages.
	FString GetStageLabel() const;

	// The zone the current (or last) battle is in, loaded from Zone_DT when it started, and which of its stages it's on.
	UPROPERTY()
	FZoneStruct Zone;

	int32 StageIndex = 0;

	// The active party as it stood when the current (or last) battle started. It keeps its health between stages.
	UPROPERTY()
	TArray<TObjectPtr<UPlayerMinion>> Minions;

	// The current stage's enemy group, spawned when the stage started.
	UPROPERTY()
	TArray<TObjectPtr<UEnemy>> Enemies;

	// Rebuilt from Minions and Enemies at the start of each stage (minions first, so they win ties).
	TArray<FBattleCombatant> Combatants;

	// Cleared at the start of each stage.
	TArray<FBattleKill> BattleKills;

	UPROPERTY()
	TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;

	UPROPERTY()
	TObjectPtr<UMinionHandler> BoundMinionHandler = nullptr;

	UPROPERTY()
	TObjectPtr<UPlayerInventory> BoundPlayerInventory = nullptr;

	UPROPERTY()
	TObjectPtr<ABattleScene> BoundBattleScene = nullptr;

	FTimerHandle BattleTickHandle;

	bool bBattleRunning = false;
	float BattleTime = 0.f;
	double BattleStartWorldTime = 0.0;

	// BattleTime when the current stage started, for timing it out after MaxStageDuration.
	float StageStartTime = 0.f;

	// The stage waiting out its pause before it starts (QueueStage), or INDEX_NONE if none is.
	int32 PendingStageIndex = INDEX_NONE;

	// BattleTime at which PendingStageIndex starts.
	float PendingStageStartTime = 0.f;
};
