// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "MinionClassStruct.h"
#include "BaseWeaponStruct.h"
#include "BaseArmorStruct.h"
#include "ItemModifierStruct.h"
#include "ItemHandler.h"
#include "PlayerMinion.generated.h"

// Broadcast whenever health, max health or overshield changes (hits, equipment, level, restores).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMinionHealthChangedEvent, float, CurrentHealth, float, MaxHealth, float, CurrentOvershield);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMinionDiedEvent);

UCLASS()
class CHAOSRECIPE_API UPlayerMinion : public UObject, public ICombatant
{
	GENERATED_BODY()

public:
	// Loads InMinionClass's row from MinionClass_DT, then recalculates stats at InLevel and restores the
	// minion to full. Returns false (leaving the minion untouched) if the class has no row.
	UFUNCTION(BlueprintCallable, Category = "Minion")
	bool InitializeMinion(EMinionClass InMinionClass, int32 InLevel = 1);

	UFUNCTION(BlueprintCallable, Category = "Minion")
	void SetLevel(int32 NewLevel);

	// Keeps equipped items in sync with the stash: a sold item is unequipped, and a rerolled one is
	// re-read so its new modifiers apply.
	UFUNCTION()
	void BindToItemHandlerEvents(UItemHandler* ItemHandler);

	// Equips a stash item by UUID, read from SavedItems.json the same way CoreMenu::PopulatePlayerStash
	// reads the stash. Replaces whatever is already in that item's slot.
	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	bool EquipItem(const FString& ItemUUID);

	// A two-handed weapon unequips any off-hand item, and an off-hand item unequips a two-handed weapon.
	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	bool EquipWeapon(const FItemWeaponStatsStruct& WeaponStats);

	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	bool EquipArmor(const FItemArmorStatsStruct& ArmorStats);

	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	bool UnequipItem(const FString& ItemUUID);

	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	void UnequipAll();

	// Re-reads every equipped item from the stash (unequipping any that are gone), e.g. after a reroll.
	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	void RefreshEquippedItems();

	UFUNCTION(BlueprintPure, Category = "Minion|Equipment")
	bool IsItemEquipped(const FString& ItemUUID) const;

	UFUNCTION(BlueprintPure, Category = "Minion|Equipment")
	TArray<FString> GetEquippedItemUUIDs() const;

	// Builds a hit from Ability and this minion's offense and applies it to Target. Returns an unresolved
	// result (bResolved=false) if either side is defeated or the minion is below Ability's RequiredLevel.
	virtual FCombatHitResult UseAbility(const FCombatAbilityStruct& Ability, ICombatTarget* Target) override;

	virtual FCombatHitResult ApplyHit(const FCombatHit& Hit) override;
	virtual bool IsDefeated() const override;

	virtual FString GetCombatantName() const override { return ClassData.ClassName.ToString(); }

	// Its basic weapon attack, plus every class ability it's high enough level for.
	virtual TArray<FCombatAbilityStruct> GetBattleAbilities() const override;

	// The equipped weapon's attack rate, or its class's AttackRate with no weapon.
	virtual float GetBattleAttackRate() const override { return Offense.AttackRate; }

	// Refills health and rolls a fresh overshield pool from equipped armor, e.g. before a fight.
	UFUNCTION(BlueprintCallable, Category = "Minion|Combat")
	void RestoreToFull();

	UFUNCTION(BlueprintPure, Category = "Minion")
	EMinionClass GetMinionClass() const { return ClassData.MinionClass; }

	UFUNCTION(BlueprintPure, Category = "Minion")
	int32 GetLevel() const { return Level; }

	// Class stats at the current level, before equipped items.
	UFUNCTION(BlueprintPure, Category = "Minion")
	FCombatAttributes GetBaseAttributes() const { return BaseAttributes; }

	// BaseAttributes after every equipped item's attribute modifiers.
	UFUNCTION(BlueprintPure, Category = "Minion")
	FCombatAttributes GetTotalAttributes() const { return TotalAttributes; }

	UFUNCTION(BlueprintPure, Category = "Minion|Combat")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Minion|Combat")
	float GetMaxHealth() const { return static_cast<float>(TotalAttributes.Health); }

	UFUNCTION(BlueprintPure, Category = "Minion|Combat")
	float GetCurrentOvershield() const { return CurrentOvershield; }

	UFUNCTION(BlueprintPure, Category = "Minion|Combat")
	FCombatOffense GetOffense() const { return Offense; }

	UFUNCTION(BlueprintPure, Category = "Minion|Combat")
	FCombatDefense GetDefense() const { return Defense; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMinionHealthChangedEvent OnHealthChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMinionDiedEvent OnDiedEvent;

protected:
	UFUNCTION()
	void HandleItemSold(FString ItemId, FString ItemUUID, float GoldValue);

	UFUNCTION()
	void HandleItemRandomized(int32 GoldCost, FString CurrencyId);

	// Rebuilds BaseAttributes, TotalAttributes, Offense and Defense from scratch out of the class row,
	// level and equipped items, so it's safe to call after any change to any of them.
	void RecalculateStats();

	bool HasWeaponEquipped() const { return !EquippedWeapon.ItemId.IsEmpty(); }
	void ClearEquippedWeapon();

	void BroadcastHealthChanged();

	UPROPERTY()
	FMinionClassStruct ClassData;

	UPROPERTY()
	int32 Level = 1;

	UPROPERTY()
	FCombatAttributes BaseAttributes;

	UPROPERTY()
	FCombatAttributes TotalAttributes;

	UPROPERTY()
	FCombatOffense Offense;

	UPROPERTY()
	FCombatDefense Defense;

	float CurrentHealth = 0.f;
	float CurrentOvershield = 0.f;

	// An empty ItemId means no weapon, the same sentinel ItemHandler uses for its cached item stats.
	UPROPERTY()
	FItemWeaponStatsStruct EquippedWeapon;

	// The equipped weapon's BaseWeapon_DT row, for its slot (two-hand check) and base crit stats.
	UPROPERTY()
	FBaseWeaponStruct EquippedWeaponBase;

	UPROPERTY()
	TMap<EArmorSlot, FItemArmorStatsStruct> EquippedArmor;

	// Every ItemModifier_DT row, loaded on first use to resolve equipped items' modifier ids.
	TArray<FItemModifierStruct> ModifierPool;
};
