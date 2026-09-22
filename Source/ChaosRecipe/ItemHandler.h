// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TagsStruct.h"
#include "BaseItemStruct.h"
#include "BaseWeaponStruct.h"
#include "BaseArmorStruct.h"
#include "ItemInstanceManager.h"
#include "ItemModifierStruct.h"
#include "ModifierAssigner.h"
#include "CurrencyManager.h"
#include "CurrencyStruct.h"
#include "ItemHandler.generated.h"

class UCoreMenu;
class UPlayerInventory;

// Broadcast by OnSellButtonClicked once it has resolved the sold item's gold value and before it
// removes the item from SavedItems.json, so listeners (e.g. PlayerInventory) always get the correct
// gold value and are guaranteed to finish handling the sale before the item is gone.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemSoldEvent, FString, ItemId, FString, ItemUUID, float, GoldValue);

// Broadcast by OnBuyButtonClicked once OnItemSelected has (re)cached the purchased item's stats, so
// listeners (e.g. PlayerInventory) can charge the player the item's current (post-modifier) gold value.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemBoughtEvent, FString, ItemId, FString, ItemUUID, float, GoldValue);

// Broadcast by OnRandomizeItem once a reroll has actually happened (i.e. there was an active item to
// reroll), so listeners (e.g. PlayerInventory) can charge the player RandomizeItemGoldCost and, if a
// currency was used (CurrencyId non-empty), consume one from that currency's stack.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRandomizedEvent, int32, GoldCost, FString, CurrencyId);

USTRUCT(BlueprintType)
struct FItemWeaponStatsStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    FText ItemId;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    FText UUID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    int32 ItemLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    FTagsStruct Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, FWeaponBaseDamage> WeaponDamage; // FString for easy look up for what damage type, FWeaponBaseDamage struct for full damage info

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, FWeaponLocalDamage> WeaponLocalDamage; // local (weapon-only) damage, added on top of base damage by mods

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    float AttackRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    int32 ItemBaseGoldValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    float ItemGoldValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, int32> ImplicitModifiers; // FString for ModifierId, int32 for modifier range roll (0 through max range for each mod)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, int32> PrefixModifiers; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, int32> SuffixModifiers; 

};

USTRUCT(BlueprintType)
struct FItemArmorStatsStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    FText ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    FText UUID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    int32 ItemLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    FTagsStruct Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    FBaseDefense BaseDefense;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    int32 ItemBaseGoldValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    float ItemGoldValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    TMap<FString, int32> ImplicitModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    TMap<FString, int32> PrefixModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Stats")
    TMap<FString, int32> SuffixModifiers;
};

UCLASS()
class CHAOSRECIPE_API UItemHandler : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void BindToCoreMenuEvents(UCoreMenu* CoreMenu);

    // Lets OnBuyButtonClicked query PlayerInventory::CanAffordGoldCost before granting a purchased
    // item, so an unaffordable buy is rejected before the item ever reaches the stash.
    UFUNCTION()
    void BindToPlayerInventory(UPlayerInventory* PlayerInventory);

    UFUNCTION()
    FItemWeaponStatsStruct GetWeaponStatsForItem(const FString& ItemId);

    UFUNCTION()
    FItemArmorStatsStruct GetArmorStatsForItem(const FString& ItemId);

    UFUNCTION()
    FText GetUUID() const;

    UFUNCTION()
    void SetUUID();

    UFUNCTION()
    void OnStashItemSelected(FString ItemId, FString ItemUUID);

    UFUNCTION()
    void OnShopItemSelected(FString ItemId, FString ItemUUID);

    UFUNCTION()
    void OnBuyButtonClicked(FString ItemId, FString ItemUUID);

    // CurrencyId may be empty (full reroll, matching the original behavior) or a valid Currency_DT row
    // id, in which case that currency's ModifierModificationType rules are applied to the item's
    // existing modifiers instead of wiping and rerolling everything.
    UFUNCTION()
    void OnRandomizeItem(FString CurrencyId);

    // Flat gold cost charged for each randomize/reroll, via OnItemRandomizedEvent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 RandomizeItemGoldCost = 25;

    // Returns true if an active item was actually rerolled (i.e. there was one cached to reroll and,
    // when CurrencyId is set, that currency actually had an effect on the item).
    UFUNCTION()
    bool RandomizeWeaponItem(const FString& CurrencyId = FString());

    // Returns true if an active item was actually rerolled (i.e. there was one cached to reroll and,
    // when CurrencyId is set, that currency actually had an effect on the item).
    UFUNCTION()
    bool RandomizeArmorItem(const FString& CurrencyId = FString());

    UFUNCTION()
    void OnSaveItem(FString ItemId);

    UFUNCTION()
    void OnSellButtonClicked(FString ItemId, FString ItemUUID);

    // Bound to CoreMenu's OnResetGameButtonClickedEvent: clears cached item stats and wipes
    // SavedItems.json (both on disk and in memory) via SavedItemsManager.
    UFUNCTION()
    void OnResetGame();

    // Reads a saved item's gold value straight from its saved JSON: itemGoldValue when present,
    // falling back to itemBaseGoldValue for saves from before itemGoldValue existed. Returns 0 if
    // ItemUUID has no saved item.
    UFUNCTION()
    float GetItemGoldValue(const FString& ItemUUID) const;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnItemSoldEvent OnItemSoldEvent;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnItemBoughtEvent OnItemBoughtEvent;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnItemRandomizedEvent OnItemRandomizedEvent;

protected:
    UPROPERTY()
    FItemWeaponStatsStruct CachedWeaponStats;

    UPROPERTY()
    FItemArmorStatsStruct CachedArmorStats;

    UPROPERTY()
    EItemClass LastSelectedItemClass = EItemClass::Weapon;

    UItemInstanceManager SavedItemsManager;

    // Receives the full pool of possible modifiers whenever an item is randomized.
    ModifierAssigner ItemModifierAssigner;

    // Resolves currency rows from Currency_DT when a randomize uses a currency.
    UCurrencyManager CurrencyManagerInstance;

    UPROPERTY()
    TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;

    UPROPERTY()
    TObjectPtr<UPlayerInventory> BoundPlayerInventory = nullptr;

    // Rebuilds CachedWeaponStats.WeaponLocalDamage (and AttackRate) from the item's base weapon data
    // plus every currently-assigned modifier, so it can be called after any partial modifier change
    // (not just a full reroll) and always reflects exactly what's on the item right now.
    void RecalculateWeaponLocalDamage();

    // Rebuilds CachedArmorStats.BaseDefense from the item's base defense plus the rolled defense modifiers.
    void RecalculateArmorDefense();

    // Applies Currency's ModifierModificationType rules (Add/Remove/RandomizeMods/RandomizeValues) to the
    // given item's modifier maps in place, respecting Currency's AffectedAffixes/CurrencyModifierTags
    // restrictions and the existing 3-prefix/3-suffix cap and bucket-exclusivity rules. RandomizeTiers and
    // Other (the exotic-only mechanics) are not yet implemented and always return false. Returns true if
    // the item's modifiers actually changed (so the caller should charge for and save/redisplay the roll).
    bool ApplyCurrencyToItemModifiers(const FCurrencyStruct& Currency, const FString& ItemId, EItemClass ItemClass,
        const FString& ItemType, TMap<FString, int32>& ImplicitModifiers, TMap<FString, int32>& PrefixModifiers,
        TMap<FString, int32>& SuffixModifiers);

    // Recomputes an item's gold value as ItemBaseGoldValue scaled by every currently-assigned modifier's
    // GoldValueModifier (product across Implicit+Prefix+Suffix), from scratch - safe to call after any
    // partial modifier change, not just a full reroll.
    float ComputeModifiedGoldValue(float ItemBaseGoldValue, const TMap<FString, int32>& ImplicitModifiers,
        const TMap<FString, int32>& PrefixModifiers, const TMap<FString, int32>& SuffixModifiers) const;

    // Shared by OnStashItemSelected/OnShopItemSelected/OnBuyButtonClicked: caches this item's stats
    // (preserving ItemUUID if given, e.g. for a saved stash item) and displays them as the active
    // item, in the same weapon/armor stat breakdown format Randomize uses.
    void OnItemSelected(const FString& ItemId, const FString& ItemUUID);

    bool LoadItemDataRow(const FString& ItemId, FBaseItemStruct& OutItemData) const;
    bool LoadWeaponDataRow(const FString& ItemId, FBaseWeaponStruct& OutWeaponData) const;
    bool LoadArmorDataRow(const FString& ItemId, FBaseArmorStruct& OutArmorData) const;

    // Loads every row from ItemModifier_DT (all possible item modifiers).
    bool LoadAllItemModifierRows(TArray<FItemModifierStruct>& OutModifiers) const;
    bool BuildWeaponStatsForItem(const FString& ItemId, FItemWeaponStatsStruct& OutWeaponStats) const;
    bool BuildArmorStatsForItem(const FString& ItemId, FItemArmorStatsStruct& OutArmorStats) const;

    void LogItemData(const FBaseItemStruct& ItemData) const;
    void LogWeaponData(const FBaseWeaponStruct& WeaponData) const;
    void LogArmorData(const FBaseArmorStruct& ArmorData) const;
};
