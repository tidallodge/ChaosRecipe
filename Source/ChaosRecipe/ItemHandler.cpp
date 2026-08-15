// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemHandler.h"
#include "CoreMenu.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"

void UItemHandler::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{
    if (!CoreMenu)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemHandler: CoreMenu reference is null."));
        return;
    }

    BoundCoreMenu = CoreMenu;
    CoreMenu->OnItemInfoButtonClickedEvent.AddDynamic(this, &UItemHandler::OnItemInfoClicked);
    CoreMenu->OnBuyButtonClickedEvent.AddDynamic(this, &UItemHandler::OnBuyButtonClicked);
    CoreMenu->OnRandomizeItemEvent.AddDynamic(this, &UItemHandler::OnRandomizeItem);
}

void UItemHandler::OnItemInfoClicked(FString ItemId)
{
    if (ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Received empty ItemId."));
        return;
    }

    FBaseItemStruct ItemData;
    if (!LoadItemDataRow(ItemId, ItemData))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No item found in BaseItem_DT for ItemId '%s'."), *ItemId);
        return;
    }

    LogItemData(ItemData);

    if (ItemData.ItemClass == EItemClass::Weapon || ItemData.ItemClass == EItemClass::Shield)
    {
        CachedWeaponStats = GetWeaponStatsForItem(ItemId);
        if (CachedWeaponStats.ItemId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Failed to cache weapon stats for ItemId '%s'."), *ItemId);
            return;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("ItemHandler cached weapon stats: ItemId=%s, UUID=%s, ItemLevel=%d, AttackRate=%.2f"),
            *CachedWeaponStats.ItemId.ToString(),
            *CachedWeaponStats.UUID.ToString(),
            CachedWeaponStats.ItemLevel,
            CachedWeaponStats.AttackRate);

        FBaseWeaponStruct WeaponData;
        if (LoadWeaponDataRow(ItemId, WeaponData))
        {
            LogWeaponData(WeaponData);
        }
    }
    else if (ItemData.ItemClass == EItemClass::Armor)
    {
        FBaseArmorStruct ArmorData;
        if (LoadArmorDataRow(ItemId, ArmorData))
        {
            LogArmorData(ArmorData);
        }
    }
}

bool UItemHandler::BuildWeaponStatsForItem(const FString& ItemId, FItemWeaponStatsStruct& OutWeaponStats) const
{
    FBaseItemStruct BaseItemData;
    FBaseWeaponStruct WeaponData;
    if (!LoadItemDataRow(ItemId, BaseItemData) || !LoadWeaponDataRow(ItemId, WeaponData))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Unable to build weapon stats for %s."), *ItemId);
        return false;
    }

    OutWeaponStats.ItemId = BaseItemData.ItemId;
    OutWeaponStats.UUID = FText::FromString(ItemId + TEXT("_UUID"));
    OutWeaponStats.ItemLevel = 1;
    OutWeaponStats.Tags = FTagsStruct();
    OutWeaponStats.AttackRate = WeaponData.WeaponBaseAttackRate;
    OutWeaponStats.WeaponDamage.Empty();
    OutWeaponStats.WeaponDamage.Add(TEXT("BaseDamage"), WeaponData.WeaponBaseDamage);
    OutWeaponStats.ImplicitModifiers.Empty();
    OutWeaponStats.PrefixModifiers.Empty();
    OutWeaponStats.SuffixModifiers.Empty();

    return true;
}

FItemWeaponStatsStruct UItemHandler::GetWeaponStatsForItem(const FString& ItemId)
{
    CachedWeaponStats = FItemWeaponStatsStruct();
    if (!BuildWeaponStatsForItem(ItemId, CachedWeaponStats))
    {
        CachedWeaponStats = FItemWeaponStatsStruct();
    }

    return CachedWeaponStats;
}

void UItemHandler::OnBuyButtonClicked(FString ItemId)
{
    UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Buy button clicked for ItemId '%s'."), *ItemId);
    CachedWeaponStats = GetWeaponStatsForItem(ItemId);
    OnItemInfoClicked(ItemId);
}

void UItemHandler::OnRandomizeItem(float RandomValue)
{
    if (RandomValue <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Randomize value must be greater than zero."));
        return;
    }

    if (CachedWeaponStats.ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No cached weapon stats available to randomize."));
        return;
    }

    CachedWeaponStats = GetWeaponStatsForItem(CachedWeaponStats.ItemId.ToString());
    ApplyRandomizedWeaponStats(RandomValue);

    FString DamageSummary;
    for (const TPair<FString, FWeaponBaseDamage>& DamageEntry : CachedWeaponStats.WeaponDamage)
    {
        if (!DamageSummary.IsEmpty())
        {
            DamageSummary += TEXT(" | ");
        }

        DamageSummary += FString::Printf(TEXT("%s:%d-%d"),
            *DamageEntry.Key,
            DamageEntry.Value.MinDamage,
            DamageEntry.Value.MaxDamage);
    }

    FString Message = FString::Printf(
        TEXT("ItemHandler randomized weapon stats:\nRandomModifierValue=%.2f\nAttackRate=%.2f\n%s"),
        RandomValue,
        CachedWeaponStats.AttackRate,
        *DamageSummary);

    if (BoundCoreMenu)
    {
        BoundCoreMenu->LogToScreen(Message);
    }

    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
}

void UItemHandler::ApplyRandomizedWeaponStats(float RandomValue)
{
    if (CachedWeaponStats.ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("No Cached Weapon Stats to apply random values to!"));
        return;
    }

    CachedWeaponStats.AttackRate *= RandomValue;

    for (TPair<FString, FWeaponBaseDamage>& DamageEntry : CachedWeaponStats.WeaponDamage)
    {
        DamageEntry.Value.MinDamage = FMath::Max(1, FMath::RoundToInt(DamageEntry.Value.MinDamage * RandomValue));
        DamageEntry.Value.MaxDamage = FMath::Max(1, FMath::RoundToInt(DamageEntry.Value.MaxDamage * RandomValue));
    }

    FString DamageSummary;
    for (const TPair<FString, FWeaponBaseDamage>& DamageEntry : CachedWeaponStats.WeaponDamage)
    {
        if (!DamageSummary.IsEmpty())
        {
            DamageSummary += TEXT(" | ");
        }

        DamageSummary += FString::Printf(TEXT("%s: %d-%d"),
            *DamageEntry.Key,
            DamageEntry.Value.MinDamage,
            DamageEntry.Value.MaxDamage);
    }

    const FString Message = FString::Printf(
        TEXT("Randomized %s: AttackRate=%.2f, Damage=[%s]"),
        *CachedWeaponStats.ItemId.ToString(),
        CachedWeaponStats.AttackRate,
        *DamageSummary);

    if (BoundCoreMenu)
    {
        BoundCoreMenu->LogToScreen(Message);
    }
}

bool UItemHandler::LoadItemDataRow(const FString& ItemId, FBaseItemStruct& OutItemData) const
{
    const FString DataTablePath = TEXT("/Game/ItemData/BaseItem_DT.BaseItem_DT");
    UDataTable* ItemDataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!ItemDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemHandler: Failed to load data table at %s."), *DataTablePath);
        return false;
    }

    const FString SearchText = ItemId;
    for (const FName& RowName : ItemDataTable->GetRowNames())
    {
        if (FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("ItemHandler::LoadItemDataRow"), true))
        {
            if (ItemRow->ItemId.ToString().Equals(SearchText, ESearchCase::IgnoreCase))
            {
                OutItemData = *ItemRow;
                return true;
            }
        }
    }

    return false;
}

bool UItemHandler::LoadWeaponDataRow(const FString& ItemId, FBaseWeaponStruct& OutWeaponData) const
{
    const FString DataTablePath = TEXT("/Game/ItemData/BaseWeapon_DT.BaseWeapon_DT");
    UDataTable* WeaponDataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!WeaponDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemHandler: Failed to load weapon data table at %s."), *DataTablePath);
        return false;
    }

    for (const FName& RowName : WeaponDataTable->GetRowNames())
    {
        if (FBaseWeaponStruct* WeaponRow = WeaponDataTable->FindRow<FBaseWeaponStruct>(RowName, TEXT("ItemHandler::LoadWeaponDataRow"), true))
        {
            if (WeaponRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
            {
                OutWeaponData = *WeaponRow;
                return true;
            }
        }
    }

    return false;
}

bool UItemHandler::LoadArmorDataRow(const FString& ItemId, FBaseArmorStruct& OutArmorData) const
{
    const FString DataTablePath = TEXT("/Game/ItemData/BaseArmor_DT.BaseArmor_DT");
    UDataTable* ArmorDataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!ArmorDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemHandler: Failed to load armor data table at %s."), *DataTablePath);
        return false;
    }

    for (const FName& RowName : ArmorDataTable->GetRowNames())
    {
        if (FBaseArmorStruct* ArmorRow = ArmorDataTable->FindRow<FBaseArmorStruct>(RowName, TEXT("ItemHandler::LoadArmorDataRow"), true))
        {
            if (ArmorRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
            {
                OutArmorData = *ArmorRow;
                return true;
            }
        }
    }

    return false;
}

void UItemHandler::LogItemData(const FBaseItemStruct& ItemData) const
{
    const FString ItemClassName = UEnum::GetValueAsString(ItemData.ItemClass);
    const FString ItemSlotName = UEnum::GetValueAsString(ItemData.ItemSlot);
    const FString ItemIconName = ItemData.ItemAssetData.ItemIcon ? ItemData.ItemAssetData.ItemIcon->GetName() : TEXT("None");
    const FString ItemStaticMeshName = ItemData.ItemAssetData.ItemStaticMesh ? ItemData.ItemAssetData.ItemStaticMesh->GetName() : TEXT("None");

    const FString ItemInfo = FString::Printf(
        TEXT("ItemId: %s\nItemName: %s\nItemClass: %s\nItemSlot: %s\nItemIcon: %s\nItemStaticMesh: %s"),
        *ItemData.ItemId.ToString(),
        *ItemData.ItemName.ToString(),
        *ItemClassName,
        *ItemSlotName,
        *ItemIconName,
        *ItemStaticMeshName);

    UE_LOG(LogTemp, Warning, TEXT("ItemHandler loaded base item data:\n%s"), *ItemInfo);
}

void UItemHandler::LogWeaponData(const FBaseWeaponStruct& WeaponData) const
{
    const FString WeaponInfo = FString::Printf(
        TEXT("WeaponType: %s\nWeaponSlot: %s\nWeaponBaseAttackRate: %.2f\nMinDamage: %d\nMaxDamage: %d"),
        *UEnum::GetValueAsString(WeaponData.WeaponType),
        *UEnum::GetValueAsString(WeaponData.WeaponSlot),
        WeaponData.WeaponBaseAttackRate,
        WeaponData.WeaponBaseDamage.MinDamage,
        WeaponData.WeaponBaseDamage.MaxDamage);

    UE_LOG(LogTemp, Warning, TEXT("ItemHandler loaded weapon table data:\n%s"), *WeaponInfo);
}

void UItemHandler::LogArmorData(const FBaseArmorStruct& ArmorData) const
{
    const FString ArmorInfo = FString::Printf(
        TEXT("ArmorType: %s\nArmorSlot: %s\nPrimaryMinDefense: %d\nPrimaryMaxDefense: %d\nSecondaryMinDefense: %d\nSecondaryMaxDefense: %d"),
        *UEnum::GetValueAsString(ArmorData.ArmorType),
        *UEnum::GetValueAsString(ArmorData.ArmorSlot),
        ArmorData.ArmorPrimaryBaseDefense.MinDefense,
        ArmorData.ArmorPrimaryBaseDefense.MaxDefense,
        ArmorData.ArmorSecondaryBaseDefense.MinDefense,
        ArmorData.ArmorSecondaryBaseDefense.MaxDefense);

    UE_LOG(LogTemp, Warning, TEXT("ItemHandler loaded armor table data:\n%s"), *ArmorInfo);
}
