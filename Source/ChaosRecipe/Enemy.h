// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "EnemyStruct.h"
#include "Enemy.generated.h"

// Broadcast whenever health or overshield changes (hits, initialization).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEnemyHealthChangedEvent, float, CurrentHealth, float, MaxHealth, float, CurrentOvershield);

// Broadcast once when the enemy dies, with the gold and currency it dropped (already rolled), so
// listeners (e.g. PlayerInventory) can grant them.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyKilledEvent, FString, EnemyId, const FEnemyLoot&, Loot);

// A simpler combatant than UPlayerMinion: no classes, levels or equipment, just the flat stats and
// spells from its Enemy_DT row.
UCLASS()
class CHAOSRECIPE_API UEnemy : public UObject, public ICombatant
{
	GENERATED_BODY()

public:
	// Loads InEnemyId's row from Enemy_DT and brings the enemy in at full health and overshield.
	// Returns false (leaving the enemy untouched) if there's no such row.
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool InitializeEnemy(const FString& InEnemyId);

	// Refills health and overshield, e.g. at the start of each battle.
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void RestoreToFull();

	// A basic attack with this enemy's Damage. Can be evaded and never crits.
	FCombatHitResult Attack(ICombatTarget* Target);

	// Casts the entry in this enemy's Spells with the given AbilityId. Returns an unresolved result
	// (bResolved=false) if there's no such spell or either side is already defeated.
	FCombatHitResult CastSpell(const FString& AbilityId, ICombatTarget* Target);

	// Builds a hit from Ability and this enemy's Damage/attack rate and applies it to Target.
	virtual FCombatHitResult UseAbility(const FCombatAbilityStruct& Ability, ICombatTarget* Target) override;

	virtual FCombatHitResult ApplyHit(const FCombatHit& Hit) override;
	virtual bool IsDefeated() const override;

	virtual FString GetCombatantName() const override { return EnemyData.EnemyName.ToString(); }

	// Its basic attack plus every entry in Spells.
	virtual TArray<FCombatAbilityStruct> GetBattleAbilities() const override;

	virtual float GetBattleAttackRate() const override { return EnemyData.AttackRate; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	FString GetEnemyId() const { return EnemyData.EnemyId.ToString(); }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	FText GetEnemyName() const { return EnemyData.EnemyName; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetMaxHealth() const { return static_cast<float>(EnemyData.MaxHealth); }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetCurrentOvershield() const { return CurrentOvershield; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetAttackRate() const { return EnemyData.AttackRate; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	TArray<FString> GetSpellIds() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	FCombatOffense GetOffense() const { return Offense; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	FCombatDefense GetDefense() const { return Defense; }

	// Feeds the CurrencyId dropdown on FEnemyCurrencyDrop in the Enemy_DT editor (GetOptions metadata).
	UFUNCTION()
	static TArray<FString> GetCurrencyIdOptions();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyHealthChangedEvent OnHealthChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyKilledEvent OnKilledEvent;

protected:
	// Rolls GoldDrop and each CurrencyDrops entry independently.
	FEnemyLoot RollLoot() const;

	void BroadcastHealthChanged();

	UPROPERTY()
	FEnemyStruct EnemyData;

	UPROPERTY()
	FCombatOffense Offense;

	UPROPERTY()
	FCombatDefense Defense;

	float CurrentHealth = 0.f;
	float CurrentOvershield = 0.f;
};
