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
#include "ItemHandler.generated.h"

class UCoreMenu;

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

    UFUNCTION()
    FItemWeaponStatsStruct GetWeaponStatsForItem(const FString& ItemId);

    UFUNCTION()
    FItemArmorStatsStruct GetArmorStatsForItem(const FString& ItemId);

    UFUNCTION()
    FText GetUUID() const;

    UFUNCTION()
    void SetUUID();

    UFUNCTION()
    void OnItemInfoClicked(FString ItemId);

    UFUNCTION()
    void OnBuyButtonClicked(FString ItemId);

    UFUNCTION()
    void OnRandomizeItem();

    UFUNCTION()
    void RandomizeWeaponItem();

    UFUNCTION()
    void OnSaveItemButtonClicked(FString ItemId);

    UFUNCTION()
    void OnSellButtonClicked(FString ItemId);

protected:
    UPROPERTY()
    FItemWeaponStatsStruct CachedWeaponStats;

    UPROPERTY()
    FItemArmorStatsStruct CachedArmorStats;

    UPROPERTY()
    EItemClass LastSelectedItemClass = EItemClass::Weapon;

    ItemInstanceManager SavedItemsManager;

    // Receives the full pool of possible modifiers whenever an item is randomized.
    ModifierAssigner ItemModifierAssigner;

    UPROPERTY()
    TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;

    // Rebuilds CachedWeaponStats.WeaponLocalDamage from the item's base damage plus the rolled damage modifiers.
    void RecalculateWeaponLocalDamage();

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
