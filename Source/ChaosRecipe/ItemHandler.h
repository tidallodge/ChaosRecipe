// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TagsStruct.h"
#include "BaseItemStruct.h"
#include "BaseWeaponStruct.h"
#include "BaseArmorStruct.h"
#include "BaseJewelryStruct.h"
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

// Broadcast by RandomizeActiveItem once a reroll has actually happened (i.e. there was an active item to
// reroll), so listeners (e.g. PlayerInventory) can charge the player RandomizeItemGoldCost and consume
// one from CurrencyId's stack.
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

// Jewelry and Misc items (BaseJewelry_Misc_DT). They have no stats of their own for modifiers to change, so every
// modifier they roll applies to whoever wears them (UPlayerMinion).
USTRUCT(BlueprintType)
struct FItemJewelryStatsStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    FText ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    FText UUID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    int32 ItemLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    FTagsStruct Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    int32 ItemBaseGoldValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    float ItemGoldValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    TMap<FString, int32> ImplicitModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
    TMap<FString, int32> PrefixModifiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jewelry Stats")
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
    FItemJewelryStatsStruct GetJewelryStatsForItem(const FString& ItemId);

    // Feeds the ItemId dropdown on FBaseWeaponStruct/FBaseArmorStruct/FBaseJewelryStruct in their data table editors
    // (GetOptions metadata): every BaseItem_DT ItemId.
    UFUNCTION()
    static TArray<FString> GetItemIdOptions();

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

    // Bound to CoreMenu's ActiveItemRollButton: rolls the active item with the currency selected in CoreMenu.
    UFUNCTION()
    void OnActiveItemRollButtonClicked();

    // Applies the ModifierModificationType rules of CurrencyId (a Currency_DT row id) to the active item's existing
    // modifiers and saves it. Rejected if no currency is given, or if the active item isn't in SavedItems.json
    // (e.g. an unbought shop item), since the roll would save it to the stash.
    UFUNCTION()
    void RandomizeActiveItem(const FString& CurrencyId);

    // Flat gold cost charged for each randomize/reroll, via OnItemRandomizedEvent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 RandomizeItemGoldCost = 25;

    // Returns true if an active item was actually rerolled (i.e. there was one cached to reroll and the
    // currency actually had an effect on the item).
    UFUNCTION()
    bool RandomizeWeaponItem(const FString& CurrencyId);

    // Returns true if an active item was actually rerolled (i.e. there was one cached to reroll and the
    // currency actually had an effect on the item).
    UFUNCTION()
    bool RandomizeArmorItem(const FString& CurrencyId);

    // Returns true if an active item was actually rerolled (i.e. there was one cached to reroll and the
    // currency actually had an effect on the item).
    UFUNCTION()
    bool RandomizeJewelryItem(const FString& CurrencyId);

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

    // The text ActiveItemTextBox shows for an item, for CoreMenu's shop/stash hover flyouts: ItemUUID's saved stats
    // (rolled modifiers and all) if it's in SavedItems.json, otherwise ItemId's base stats, as the shop lists it.
    // Empty if the item can't be found.
    FString GetItemDisplayText(const FString& ItemId, const FString& ItemUUID);

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
    FItemJewelryStatsStruct CachedJewelryStats;

    // Which cache holds the active item: Weapon (CachedWeaponStats), Armor (CachedArmorStats, Shields too) or
    // Jewelry (CachedJewelryStats, Misc items too).
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
    // plus every currently-assigned Local modifier, so it can be called after any modifier change and always
    // reflects exactly what's on the item right now.
    void RecalculateWeaponLocalDamage();

    // Rebuilds CachedArmorStats.BaseDefense from the item's base defense plus the rolled Local defense modifiers.
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
    // modifier change.
    float ComputeModifiedGoldValue(float ItemBaseGoldValue, const TMap<FString, int32>& ImplicitModifiers,
        const TMap<FString, int32>& PrefixModifiers, const TMap<FString, int32>& SuffixModifiers) const;

    // Shared by OnStashItemSelected/OnShopItemSelected/OnBuyButtonClicked: caches this item's stats
    // (preserving ItemUUID if given) and displays them as the active item, in the same stat breakdown
    // format Randomize uses. An item saved in SavedItems.json (i.e. a stash item) is cached as it was
    // saved, rolled modifiers and all; anything else (e.g. a shop listing) gets its base stats. Returns
    // false, leaving nothing new cached, if the item's stats can't be built.
    bool OnItemSelected(const FString& ItemId, const FString& ItemUUID);

    // The UUID of the active item, in whichever cache LastSelectedItemClass says holds it.
    FText& GetActiveItemUUID();
    const FText& GetActiveItemUUID() const { return const_cast<UItemHandler*>(this)->GetActiveItemUUID(); }

    // The active item's current (post-modifier) gold value, from whichever cache holds it.
    float GetActiveItemGoldValue() const;

    // Saves the active item, from whichever cache holds it, to SavedItems.json under ItemUUID.
    void SaveActiveItem(const FString& ItemUUID);

    // Loads ItemModifier_DT into ItemModifierAssigner if it isn't yet. Randomizing is what normally loads it, so
    // without this a saved item's modifiers would show as raw ids until something had been randomized this session.
    void EnsureModifierPoolLoaded();

    // The ActiveItemTextBox text for an item, shared by OnItemSelected, Randomize*Item and GetItemDisplayText so
    // they all lay it out the same way: its name, a "Base Damage:"/"Base Defense:" block (jewelry has neither),
    // Attack Rate (weapons only), Gold Value, then its modifiers (named from ItemModifierAssigner's pool).
    FString BuildWeaponDisplayText(const FString& ItemName, const FItemWeaponStatsStruct& WeaponStats) const;
    FString BuildArmorDisplayText(const FString& ItemName, const FItemArmorStatsStruct& ArmorStats) const;
    FString BuildJewelryDisplayText(const FString& ItemName, const FItemJewelryStatsStruct& JewelryStats) const;

    bool LoadItemDataRow(const FString& ItemId, FBaseItemStruct& OutItemData) const;
    bool LoadWeaponDataRow(const FString& ItemId, FBaseWeaponStruct& OutWeaponData) const;
    bool LoadArmorDataRow(const FString& ItemId, FBaseArmorStruct& OutArmorData) const;
    bool LoadJewelryDataRow(const FString& ItemId, FBaseJewelryStruct& OutJewelryData) const;

    // Loads every row from ItemModifier_DT (all possible item modifiers).
    bool LoadAllItemModifierRows(TArray<FItemModifierStruct>& OutModifiers) const;
    bool BuildWeaponStatsForItem(const FString& ItemId, FItemWeaponStatsStruct& OutWeaponStats) const;
    bool BuildArmorStatsForItem(const FString& ItemId, FItemArmorStatsStruct& OutArmorStats) const;
    bool BuildJewelryStatsForItem(const FString& ItemId, FItemJewelryStatsStruct& OutJewelryStats) const;

    void LogItemData(const FBaseItemStruct& ItemData) const;
    void LogWeaponData(const FBaseWeaponStruct& WeaponData) const;
    void LogArmorData(const FBaseArmorStruct& ArmorData) const;
    void LogJewelryData(const FBaseJewelryStruct& JewelryData) const;
};
