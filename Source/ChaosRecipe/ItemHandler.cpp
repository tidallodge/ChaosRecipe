// ItemHandler
/** Purpose: Compile a full item across mutliple Data Tables
    Assign a new item its UUID and base stats
    Assign item modifiers and update stats accordingly
    
*/


#include "ItemHandler.h"
#include "CoreMenu.h"
#include "PlayerInventory.h"
#include "Engine/DataTable.h"
#include "TagsStruct.h"
#include "BaseItemStruct.h"
#include "BaseWeaponStruct.h"
#include "BaseArmorStruct.h"
#include "ItemInstanceManager.h"
#include "Engine/Engine.h"

namespace
{
	// Builds the modifiers text for ActiveItemTextBox, matching the layout CoreMenu uses when a
	// stash item is selected (OnSingleLoadItemButtonClicked): one "<AffixType>:" header per non-empty
	// group (Implicit, then Prefixes, then Suffixes), each followed by indented "  Name (value)"
	// lines, and falling back to "Modifiers: none\n" when there are no modifiers at all. Looks each
	// ModifierId up in ModifierPool to display its ModifierName instead of the raw id.
	FString BuildGroupedModifiersText(const TArray<FItemModifierStruct>& ModifierPool, const TMap<FString, int32>& ImplicitModifiers, const TMap<FString, int32>& PrefixModifiers, const TMap<FString, int32>& SuffixModifiers)
	{
		FString Text;
		auto AppendGroup = [&Text, &ModifierPool](const TMap<FString, int32>& Modifiers, const TCHAR* GroupLabel)
		{
			if (Modifiers.Num() == 0)
			{
				return;
			}

			Text += FString::Printf(TEXT("%s:\n"), GroupLabel);
			for (const TPair<FString, int32>& Entry : Modifiers)
			{
				const FItemModifierStruct* ModifierRow = ModifierPool.FindByPredicate(
					[&Entry](const FItemModifierStruct& Modifier)
					{
						return Modifier.ModifierId.ToString().Equals(Entry.Key, ESearchCase::IgnoreCase);
					});
				const FString DisplayName = ModifierRow ? ModifierRow->ModifierName.ToString() : Entry.Key;

				Text += FString::Printf(TEXT("  %s (%d)\n"), *DisplayName, Entry.Value);
			}
		};

		AppendGroup(ImplicitModifiers, TEXT("Implicit"));
		AppendGroup(PrefixModifiers, TEXT("Prefixes"));
		AppendGroup(SuffixModifiers, TEXT("Suffixes"));

		if (Text.IsEmpty())
		{
			Text = TEXT("Modifiers: none\n");
		}

		return Text;
	}

	// Formats each non-zero local damage channel as an indented "  Label: min-max\n" line, for the
	// "Base Damage:" section of ActiveItemText (matches the stash-item display's layout).
	FString BuildWeaponDamageLines(const FWeaponLocalDamage& LocalDamage)
	{
		struct FLocalDamageLine { const TCHAR* Label; FIntPoint Range; };
		const FLocalDamageLine LocalLines[] = {
			{ TEXT("Physical"), LocalDamage.LocalPhysicalDamage },
			{ TEXT("Fire"),     LocalDamage.LocalFireDamage },
			{ TEXT("Ice"),      LocalDamage.LocalIceDamage },
			{ TEXT("Electric"), LocalDamage.LocalElectricDamage },
			{ TEXT("Abyssal"),  LocalDamage.LocalAbyssalDamage },
		};

		FString Text;
		for (const FLocalDamageLine& Line : LocalLines)
		{
			if (Line.Range.X == 0 && Line.Range.Y == 0)
			{
				continue;
			}

			Text += FString::Printf(TEXT("  %s: %d-%d\n"), Line.Label, Line.Range.X, Line.Range.Y);
		}
		return Text;
	}

	// Formats defense as indented "  Label: min-max\n" lines, for the "Base Defense:" section of
	// ActiveItemText (armor's equivalent of BuildWeaponDamageLines).
	FString BuildArmorDefenseLines(const FBaseDefense& Defense)
	{
		return FString::Printf(
			TEXT("  Physical Mitigation: %d-%d\n  Evade: %d-%d\n  Overshield: %d-%d\n"),
			Defense.BasePhysicalMitigation.X, Defense.BasePhysicalMitigation.Y,
			Defense.BaseEvade.X, Defense.BaseEvade.Y,
			Defense.BaseOvershield.X, Defense.BaseOvershield.Y);
	}

	// ItemGoldValue is populated as soon as base stats are built (it starts equal to ItemBaseGoldValue),
	// so the ItemBaseGoldValue fallback only matters for stats structs that predate this field.
	template <typename TItemStats>
	float GetDisplayGoldValue(const TItemStats& ItemStats)
	{
		return ItemStats.ItemGoldValue > 0.f ? ItemStats.ItemGoldValue : static_cast<float>(ItemStats.ItemBaseGoldValue);
	}
}

void UItemHandler::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{
    if (!CoreMenu)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemHandler: CoreMenu reference is null."));
        return;
    }

    BoundCoreMenu = CoreMenu;
    CoreMenu->OnBuyButtonClickedEvent.AddDynamic(this, &UItemHandler::OnBuyButtonClicked);
    CoreMenu->OnRandomizeItemEvent.AddDynamic(this, &UItemHandler::OnRandomizeItem);
    CoreMenu->OnSellButtonClickedEvent.AddDynamic(this, &UItemHandler::OnSellButtonClicked);
    CoreMenu->OnStashItemSelectedEvent.AddDynamic(this, &UItemHandler::OnStashItemSelected);
    CoreMenu->OnShopItemSelectedEvent.AddDynamic(this, &UItemHandler::OnShopItemSelected);
    CoreMenu->OnResetGameButtonClickedEvent.AddDynamic(this, &UItemHandler::OnResetGame);
}

void UItemHandler::BindToPlayerInventory(UPlayerInventory* PlayerInventory)
{
    if (!PlayerInventory)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemHandler: PlayerInventory reference is null."));
        return;
    }

    BoundPlayerInventory = PlayerInventory;
}

void UItemHandler::OnStashItemSelected(FString ItemId, FString ItemUUID)
{
    if (ItemId.IsEmpty() || ItemUUID.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Received empty ItemId/UUID for stash selection."));
        return;
    }

    OnItemSelected(ItemId, ItemUUID);
}

void UItemHandler::OnShopItemSelected(FString ItemId, FString ItemUUID)
{
    // ItemUUID was already minted for this listing back in CoreMenu::PopulateShopGrid.
    OnItemSelected(ItemId, ItemUUID);
}

void UItemHandler::OnItemSelected(const FString& ItemId, const FString& ItemUUID)
{
    if (ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Received empty ItemId for item selection."));
        return;
    }

    FBaseItemStruct ItemData;
    if (!LoadItemDataRow(ItemId, ItemData))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No item found in BaseItem_DT for ItemId '%s'."), *ItemId);
        return;
    }

    LogItemData(ItemData);

    const bool bIsArmor = ItemData.ItemClass == EItemClass::Armor || ItemData.ItemClass == EItemClass::Shield;
    LastSelectedItemClass = bIsArmor ? EItemClass::Armor : EItemClass::Weapon;

    // Pre-seed the relevant cache with an existing UUID (e.g. a saved stash item) so the
    // GetWeaponStatsForItem/GetArmorStatsForItem call below preserves it instead of leaving it
    // empty (which would make a later Save mint a brand new UUID rather than overwriting this item).
    if (!ItemUUID.IsEmpty())
    {
        if (bIsArmor)
        {
            CachedArmorStats.UUID = FText::FromString(ItemUUID);
        }
        else
        {
            CachedWeaponStats.UUID = FText::FromString(ItemUUID);
        }
    }

    const FString ItemName = ItemData.ItemName.ToString();
    FString Message;
    FString ActiveItemText;

    if (!bIsArmor)
    {
        CachedWeaponStats = GetWeaponStatsForItem(ItemId);
        if (CachedWeaponStats.ItemId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Failed to cache weapon stats for ItemId '%s'."), *ItemId);
            return;
        }

        FBaseWeaponStruct WeaponData;
        if (LoadWeaponDataRow(ItemId, WeaponData))
        {
            LogWeaponData(WeaponData);
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
                DamageEntry.Value.BasePhysicalDamage.X,
                DamageEntry.Value.BasePhysicalDamage.Y);
        }

        FString LocalDamageSummary;
        for (const TPair<FString, FWeaponLocalDamage>& LocalEntry : CachedWeaponStats.WeaponLocalDamage)
        {
            struct FLocalDamageLine { const TCHAR* Label; FIntPoint Range; };
            const FLocalDamageLine LocalLines[] = {
                { TEXT("Physical"), LocalEntry.Value.LocalPhysicalDamage },
                { TEXT("Fire"),     LocalEntry.Value.LocalFireDamage },
                { TEXT("Ice"),      LocalEntry.Value.LocalIceDamage },
                { TEXT("Electric"), LocalEntry.Value.LocalElectricDamage },
                { TEXT("Abyssal"),   LocalEntry.Value.LocalAbyssalDamage },
            };

            for (const FLocalDamageLine& LocalLine : LocalLines)
            {
                if (LocalLine.Range.X == 0 && LocalLine.Range.Y == 0)
                {
                    continue;
                }

                if (!LocalDamageSummary.IsEmpty())
                {
                    LocalDamageSummary += TEXT(" | ");
                }

                LocalDamageSummary += FString::Printf(TEXT("%s:%d-%d"),
                    LocalLine.Label, LocalLine.Range.X, LocalLine.Range.Y);
            }
        }

        if (LocalDamageSummary.IsEmpty())
        {
            LocalDamageSummary = TEXT("none");
        }

        const FString ModifiersText = BuildGroupedModifiersText(ItemModifierAssigner.GetModifierPool(),
            CachedWeaponStats.ImplicitModifiers, CachedWeaponStats.PrefixModifiers, CachedWeaponStats.SuffixModifiers);

        Message = FString::Printf(
            TEXT("ItemHandler selected weapon stats:\nUUID:%s\nAttackRate=%.2f\nBase:%s\nLocal:%s"),
            *CachedWeaponStats.UUID.ToString(),
            CachedWeaponStats.AttackRate,
            *DamageSummary,
            *LocalDamageSummary);

        // Matches the layout CoreMenu uses for a selected stash item: name, then a "Base Damage:"
        // block, Attack Rate, Gold Value, then the modifiers block.
        FString WeaponDamageText;
        if (const FWeaponLocalDamage* ActiveLocalDamage = CachedWeaponStats.WeaponLocalDamage.Find(TEXT("LocalDamage")))
        {
            WeaponDamageText = BuildWeaponDamageLines(*ActiveLocalDamage);
        }

        ActiveItemText = FString::Printf(
            TEXT("%s\nBase Damage:\n%sAttack Rate: %.2f\nGold Value: %.0f\n%s"),
            *ItemName,
            *WeaponDamageText,
            CachedWeaponStats.AttackRate,
            GetDisplayGoldValue(CachedWeaponStats),
            *ModifiersText);
    }
    else
    {
        FBaseArmorStruct ArmorData;
        if (LoadArmorDataRow(ItemId, ArmorData))
        {
            LogArmorData(ArmorData);
        }

        CachedArmorStats = GetArmorStatsForItem(ItemId);
        if (CachedArmorStats.ItemId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Failed to cache armor stats for ItemId '%s'."), *ItemId);
            return;
        }

        const FString DefenseSummary = FString::Printf(
            TEXT("PhysicalMitigation:%d-%d | Evade:%d-%d | Overshield:%d-%d"),
            CachedArmorStats.BaseDefense.BasePhysicalMitigation.X,
            CachedArmorStats.BaseDefense.BasePhysicalMitigation.Y,
            CachedArmorStats.BaseDefense.BaseEvade.X,
            CachedArmorStats.BaseDefense.BaseEvade.Y,
            CachedArmorStats.BaseDefense.BaseOvershield.X,
            CachedArmorStats.BaseDefense.BaseOvershield.Y);

        const FString ModifiersText = BuildGroupedModifiersText(ItemModifierAssigner.GetModifierPool(),
            CachedArmorStats.ImplicitModifiers, CachedArmorStats.PrefixModifiers, CachedArmorStats.SuffixModifiers);

        Message = FString::Printf(
            TEXT("ItemHandler selected armor stats:\nUUID:%s\nDefense:%s"),
            *CachedArmorStats.UUID.ToString(),
            *DefenseSummary);

        // Matches the layout CoreMenu uses for a selected stash item: name, then a "Base Defense:"
        // block, Gold Value, then the modifiers block.
        ActiveItemText = FString::Printf(
            TEXT("%s\nBase Defense:\n%sGold Value: %.0f\n%s"),
            *ItemName,
            *BuildArmorDefenseLines(CachedArmorStats.BaseDefense),
            GetDisplayGoldValue(CachedArmorStats),
            *ModifiersText);
    }

    if (BoundCoreMenu)
    {
        BoundCoreMenu->LogToScreen(Message);
        BoundCoreMenu->SetActiveItemText(ActiveItemText);
    }

    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
}

bool UItemHandler::BuildWeaponStatsForItem(const FString& ItemId, FItemWeaponStatsStruct& OutWeaponStats) const
{
    const FText ExistingUUID = OutWeaponStats.UUID;

    FBaseItemStruct BaseItemData; 
    FBaseWeaponStruct WeaponData;
    if (!LoadItemDataRow(ItemId, BaseItemData) || !LoadWeaponDataRow(ItemId, WeaponData))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Unable to build weapon stats for %s."), *ItemId);
        return false;
    }

    OutWeaponStats.ItemId = BaseItemData.ItemId;
    OutWeaponStats.ItemLevel = 1;
    OutWeaponStats.Tags = FTagsStruct();
    OutWeaponStats.AttackRate = WeaponData.WeaponBaseAttackRate;
    OutWeaponStats.ItemBaseGoldValue = WeaponData.ItemBaseGoldValue;
    OutWeaponStats.ItemGoldValue = WeaponData.ItemBaseGoldValue;
    OutWeaponStats.WeaponDamage.Empty();
    OutWeaponStats.WeaponDamage.Add(TEXT("BaseDamage"), WeaponData.WeaponBaseDamage);

    // Seed local damage from the item's base damage (i.e. no modifiers rolled yet) rather than
    // WeaponData.WeaponLocalDamage, which is a separate, normally-unset data table field -
    // RandomizeWeaponItem is what actually derives local damage from rolled modifiers afterward.
    FWeaponLocalDamage InitialLocalDamage;
    InitialLocalDamage.LocalPhysicalDamage = WeaponData.WeaponBaseDamage.BasePhysicalDamage;
    InitialLocalDamage.LocalFireDamage = WeaponData.WeaponBaseDamage.BaseFireDamage;
    InitialLocalDamage.LocalIceDamage = WeaponData.WeaponBaseDamage.BaseIceDamage;
    InitialLocalDamage.LocalElectricDamage = WeaponData.WeaponBaseDamage.BaseElectricDamage;
    InitialLocalDamage.LocalAbyssalDamage = WeaponData.WeaponBaseDamage.BaseAbyssalDamage;
    OutWeaponStats.WeaponLocalDamage.Empty();
    OutWeaponStats.WeaponLocalDamage.Add(TEXT("LocalDamage"), InitialLocalDamage);
    OutWeaponStats.ImplicitModifiers.Empty();
    OutWeaponStats.PrefixModifiers.Empty();
    OutWeaponStats.SuffixModifiers.Empty();

    if (!ExistingUUID.IsEmpty())
    {
        OutWeaponStats.UUID = ExistingUUID;
    }

    return true;
}

FItemWeaponStatsStruct UItemHandler::GetWeaponStatsForItem(const FString& ItemId)
{
    const FText ExistingUUID = CachedWeaponStats.UUID;
    CachedWeaponStats = FItemWeaponStatsStruct();
    CachedWeaponStats.UUID = ExistingUUID;

    if (!BuildWeaponStatsForItem(ItemId, CachedWeaponStats))
    {
        CachedWeaponStats = FItemWeaponStatsStruct();
    }

    return CachedWeaponStats;
}

bool UItemHandler::BuildArmorStatsForItem(const FString& ItemId, FItemArmorStatsStruct& OutArmorStats) const
{
    const FText ExistingUUID = OutArmorStats.UUID;

    FBaseItemStruct BaseItemData;
    FBaseArmorStruct ArmorData;
    if (!LoadItemDataRow(ItemId, BaseItemData) || !LoadArmorDataRow(ItemId, ArmorData))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Unable to build armor stats for %s."), *ItemId);
        return false;
    }

    OutArmorStats.ItemId = BaseItemData.ItemId;
    OutArmorStats.ItemLevel = 1;
    OutArmorStats.Tags = FTagsStruct();
    OutArmorStats.BaseDefense = ArmorData.BaseDefense;
    OutArmorStats.ItemBaseGoldValue = ArmorData.ItemBaseGoldValue;
    OutArmorStats.ItemGoldValue = ArmorData.ItemBaseGoldValue;
    OutArmorStats.ImplicitModifiers.Empty();
    OutArmorStats.PrefixModifiers.Empty();
    OutArmorStats.SuffixModifiers.Empty();

    if (!ExistingUUID.IsEmpty())
    {
        OutArmorStats.UUID = ExistingUUID;
    }

    return true;
}

FItemArmorStatsStruct UItemHandler::GetArmorStatsForItem(const FString& ItemId)
{
    const FText ExistingUUID = CachedArmorStats.UUID;
    CachedArmorStats = FItemArmorStatsStruct();
    CachedArmorStats.UUID = ExistingUUID;

    if (!BuildArmorStatsForItem(ItemId, CachedArmorStats))
    {
        CachedArmorStats = FItemArmorStatsStruct();
    }

    return CachedArmorStats;
}

FText UItemHandler::GetUUID() const
{
    if (LastSelectedItemClass == EItemClass::Armor)
    {
        if (CachedArmorStats.UUID.IsEmpty())
        {
            const_cast<UItemHandler*>(this)->SetUUID();
        }
        return CachedArmorStats.UUID;
    }

    if (CachedWeaponStats.UUID.IsEmpty())
    {
        const_cast<UItemHandler*>(this)->SetUUID();
    }

    return CachedWeaponStats.UUID;
}

void UItemHandler::SetUUID()
{
    const FText NewUUID = FText::FromString(FGuid::NewGuid().ToString());
    if (LastSelectedItemClass == EItemClass::Armor)
    {
        CachedArmorStats.UUID = NewUUID;
    }
    else
    {
        CachedWeaponStats.UUID = NewUUID;
    }
}

void UItemHandler::OnBuyButtonClicked(FString ItemId, FString ItemUUID)
{
    UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Buy button clicked for ItemId '%s'."), *ItemId);
    // UUID was already minted for this listing in PopulateShopGrid and cached on selection
    // (OnItemSelected's ExistingUUID preservation below), so Buy no longer mints one itself.
    OnItemSelected(ItemId, ItemUUID);

    // OnItemSelected just refreshed whichever cache matches LastSelectedItemClass, so read the
    // purchased item's gold value (post-modifier, same value shown in ActiveItemText) from there.
    const float BoughtGoldValue = LastSelectedItemClass == EItemClass::Armor
        ? GetDisplayGoldValue(CachedArmorStats)
        : GetDisplayGoldValue(CachedWeaponStats);

    const int32 RoundedGoldCost = FMath::RoundToInt(BoughtGoldValue);
    if (BoundPlayerInventory && !BoundPlayerInventory->CanAffordGoldCost(RoundedGoldCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Player does not have enough gold to buy item '%s' (cost=%d)."), *ItemId, RoundedGoldCost);
        if (BoundCoreMenu)
        {
            BoundCoreMenu->SetWarningText(TEXT("Not enough gold to complete action!"));
        }
        return;
    }

    // Add the purchased item to the player's stash the same way Save does, so it shows up in
    // PlayerStashUniGrid (backed by SavedItems.json) without requiring a separate Save click.
    if (LastSelectedItemClass == EItemClass::Armor)
    {
        SavedItemsManager.SaveItem(ItemUUID, CachedArmorStats);
    }
    else
    {
        SavedItemsManager.SaveItem(ItemUUID, CachedWeaponStats);
    }

    OnItemBoughtEvent.Broadcast(ItemId, ItemUUID, BoughtGoldValue);

    // Only remove the listing from the shop now that the purchase has actually gone through.
    if (BoundCoreMenu)
    {
        BoundCoreMenu->RemoveItemFromShop(ItemId);
    }
}

void UItemHandler::OnRandomizeItem()
{
    if (BoundPlayerInventory && !BoundPlayerInventory->CanAffordGoldCost(RandomizeItemGoldCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Player does not have enough gold to randomize (cost=%d)."), RandomizeItemGoldCost);
        if (BoundCoreMenu)
        {
            BoundCoreMenu->SetWarningText(TEXT("Not enough gold to complete action!"));
        }
        return;
    }

    bool bDidRandomize = false;
    if (LastSelectedItemClass == EItemClass::Weapon)
    {
        bDidRandomize = RandomizeWeaponItem();
    }
    else if (LastSelectedItemClass == EItemClass::Armor)
    {
        bDidRandomize = RandomizeArmorItem();
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ItemHandler: Randomizing item class '%s' is not supported yet."),
            *UEnum::GetValueAsString(LastSelectedItemClass));
    }

    if (bDidRandomize)
    {
        OnItemRandomizedEvent.Broadcast(RandomizeItemGoldCost);
    }
}

bool UItemHandler::RandomizeWeaponItem()
{
    if (CachedWeaponStats.ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No cached weapon stats available to randomize."));
        return false;
    }

    // Load every possible item modifier and hand the full pool to the ModifierAssigner.
    TArray<FItemModifierStruct> AllItemModifiers;
    if (LoadAllItemModifierRows(AllItemModifiers))
    {
        ItemModifierAssigner.SetModifierPool(AllItemModifiers);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Failed to load modifiers from ItemModifier_DT."));
    }

    CachedWeaponStats = GetWeaponStatsForItem(CachedWeaponStats.ItemId.ToString());

    // Roll how many modifiers this item gets: 3-6, weighted so 4 and 5 are the common outcomes.
    {
        static const int32 ModifierCountChoices[] = { 3, 4, 4, 5, 5, 6 };
        ItemModifierAssigner.ModifierCount = ModifierCountChoices[FMath::RandRange(0, UE_ARRAY_COUNT(ModifierCountChoices) - 1)];
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: rolled ModifierCount=%d for this randomize."), ItemModifierAssigner.ModifierCount);
    }

    // Resolve this item's class and type, then let the ModifierAssigner pick the modifiers.
    const FString RandomizeItemId = CachedWeaponStats.ItemId.ToString();
    FString RandomizeItemName = RandomizeItemId;
    FString ModifiersText;
    FBaseItemStruct RandomizeItemData;
    FBaseWeaponStruct RandomizeWeaponData;
    if (LoadItemDataRow(RandomizeItemId, RandomizeItemData) && LoadWeaponDataRow(RandomizeItemId, RandomizeWeaponData))
    {
        RandomizeItemName = RandomizeItemData.ItemName.ToString();
        const EItemClass RandomizeItemClass = RandomizeItemData.ItemClass;
        const FString RandomizeItemType = UEnum::GetValueAsString(RandomizeWeaponData.WeaponType)
            .RightChop(FString(TEXT("EWeaponType::")).Len());

        const TArray<FString> AssignedModifierIds =
            ItemModifierAssigner.AssignModifiers(RandomizeItemId, RandomizeItemClass, RandomizeItemType, ItemModifierAssigner.ModifierCount);

        CachedWeaponStats.ImplicitModifiers.Empty();
        CachedWeaponStats.PrefixModifiers.Empty();
        CachedWeaponStats.SuffixModifiers.Empty();

        float GoldValueMultiplier = 1.f;
        for (const FString& ModifierId : AssignedModifierIds)
        {
            const FItemModifierStruct* ModifierRow = ItemModifierAssigner.GetModifierPool().FindByPredicate(
                [&ModifierId](const FItemModifierStruct& Modifier)
                {
                    return Modifier.ModifierId.ToString().Equals(ModifierId, ESearchCase::IgnoreCase);
                });

            int32 RolledValue = 0;
            EAffixType AffixType = EAffixType::Prefix;
            if (ModifierRow)
            {
                AffixType = ModifierRow->ModifierAffixType;
                if (ModifierRow->MinMaxRange.Num() >= 2)
                {
                    RolledValue = FMath::RandRange(ModifierRow->MinMaxRange[0], ModifierRow->MinMaxRange[1]);
                }
                else if (ModifierRow->MinMaxRange.Num() == 1)
                {
                    RolledValue = ModifierRow->MinMaxRange[0];
                }
            }

            switch (AffixType)
            {
            case EAffixType::Implicit: CachedWeaponStats.ImplicitModifiers.Add(ModifierId, RolledValue); break;
            case EAffixType::Suffix:   CachedWeaponStats.SuffixModifiers.Add(ModifierId, RolledValue);   break;
            case EAffixType::Prefix:
            default:                   CachedWeaponStats.PrefixModifiers.Add(ModifierId, RolledValue);   break;
            }

            // Attack speed modifiers scale the base attack rate by their rolled percentage.
            if (ModifierRow && ModifierRow->ModifiedAttribute.WeaponAttackRate != 0.f)
            {
                CachedWeaponStats.AttackRate *= 1.f + (RolledValue / 100.f);
            }

            if (ModifierRow)
            {
                GoldValueMultiplier *= ModifierRow->GoldValueModifier;
            }

            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: assigned modifier %s (%s) value=%d"),
                *ModifierId, *UEnum::GetValueAsString(AffixType), RolledValue);
        }

        // ItemGoldValue is the item's base gold value scaled by every rolled modifier's GoldValueModifier.
        CachedWeaponStats.ItemGoldValue = CachedWeaponStats.ItemBaseGoldValue * GoldValueMultiplier;

        // List prefixes before suffixes in the summary text, regardless of roll order.
        ModifiersText = BuildGroupedModifiersText(ItemModifierAssigner.GetModifierPool(),
            CachedWeaponStats.ImplicitModifiers, CachedWeaponStats.PrefixModifiers, CachedWeaponStats.SuffixModifiers);

        // Fold the rolled damage modifiers into the item's local damage values.
        RecalculateWeaponLocalDamage();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Could not resolve class/type for '%s' to assign modifiers."), *RandomizeItemId);
    }

    SavedItemsManager.SaveItem(GetUUID().ToString(), CachedWeaponStats);

    FString DamageSummary;
    for (const TPair<FString, FWeaponBaseDamage>& DamageEntry : CachedWeaponStats.WeaponDamage)
    {
        if (!DamageSummary.IsEmpty())
        {
            DamageSummary += TEXT(" | ");
        }

        DamageSummary += FString::Printf(TEXT("%s: %d-%d"),
            *DamageEntry.Key,
            DamageEntry.Value.BasePhysicalDamage.X,
            DamageEntry.Value.BasePhysicalDamage.Y);
    }

    FString LocalDamageSummary;
    for (const TPair<FString, FWeaponLocalDamage>& LocalEntry : CachedWeaponStats.WeaponLocalDamage)
    {
        struct FLocalDamageLine { const TCHAR* Label; FIntPoint Range; };
        const FLocalDamageLine LocalLines[] = {
            { TEXT("Physical"), LocalEntry.Value.LocalPhysicalDamage },
            { TEXT("Fire"),     LocalEntry.Value.LocalFireDamage },
            { TEXT("Ice"),      LocalEntry.Value.LocalIceDamage },
            { TEXT("Electric"), LocalEntry.Value.LocalElectricDamage },
            { TEXT("Abyssal"),   LocalEntry.Value.LocalAbyssalDamage },
        };

        for (const FLocalDamageLine& LocalLine : LocalLines)
        {
            if (LocalLine.Range.X == 0 && LocalLine.Range.Y == 0)
            {
                continue;
            }

            if (!LocalDamageSummary.IsEmpty())
            {
                LocalDamageSummary += TEXT(" | ");
            }

            LocalDamageSummary += FString::Printf(TEXT("%s:%d-%d"),
                LocalLine.Label, LocalLine.Range.X, LocalLine.Range.Y);
        }
    }

    if (LocalDamageSummary.IsEmpty())
    {
        LocalDamageSummary = TEXT("none");
    }

    FString Message = FString::Printf(
        TEXT("ItemHandler randomized weapon stats:\nUUID:%s\nAttackRate=%.2f\nBase:%s\nLocal:%s"),
        *CachedWeaponStats.UUID.ToString(),
        CachedWeaponStats.AttackRate,
        *DamageSummary,
        *LocalDamageSummary);

    if (ModifiersText.IsEmpty())
    {
        ModifiersText = TEXT("Modifiers: none\n");
    }

    // Matches the layout CoreMenu uses for a selected stash item: name, then a "Base Damage:"
    // block, Attack Rate, Gold Value, then the modifiers block.
    FString WeaponDamageText;
    if (const FWeaponLocalDamage* ActiveLocalDamage = CachedWeaponStats.WeaponLocalDamage.Find(TEXT("LocalDamage")))
    {
        WeaponDamageText = BuildWeaponDamageLines(*ActiveLocalDamage);
    }

    const FString ActiveItemText = FString::Printf(
        TEXT("%s\nBase Damage:\n%sAttack Rate: %.2f\nGold Value: %.0f\n%s"),
        *RandomizeItemName,
        *WeaponDamageText,
        CachedWeaponStats.AttackRate,
        GetDisplayGoldValue(CachedWeaponStats),
        *ModifiersText);

    if (BoundCoreMenu)
    {
        BoundCoreMenu->LogToScreen(Message);
        BoundCoreMenu->SetActiveItemText(ActiveItemText);
    }

    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

    return true;
}

bool UItemHandler::RandomizeArmorItem()
{
    if (CachedArmorStats.ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No cached armor stats available to randomize."));
        return false;
    }

    // Load every possible item modifier and hand the full pool to the ModifierAssigner.
    TArray<FItemModifierStruct> AllItemModifiers;
    if (LoadAllItemModifierRows(AllItemModifiers))
    {
        ItemModifierAssigner.SetModifierPool(AllItemModifiers);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Failed to load modifiers from ItemModifier_DT."));
    }

    CachedArmorStats = GetArmorStatsForItem(CachedArmorStats.ItemId.ToString());

    // Roll how many modifiers this item gets: 3-6, weighted so 4 and 5 are the common outcomes.
    {
        static const int32 ModifierCountChoices[] = { 3, 4, 4, 5, 5, 6 };
        ItemModifierAssigner.ModifierCount = ModifierCountChoices[FMath::RandRange(0, UE_ARRAY_COUNT(ModifierCountChoices) - 1)];
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: rolled ModifierCount=%d for this randomize."), ItemModifierAssigner.ModifierCount);
    }

    // Resolve this item's class and type, then let the ModifierAssigner pick the modifiers.
    const FString RandomizeItemId = CachedArmorStats.ItemId.ToString();
    FString RandomizeItemName = RandomizeItemId;
    FString ModifiersText;
    FBaseItemStruct RandomizeItemData;
    FBaseArmorStruct RandomizeArmorData;
    if (LoadItemDataRow(RandomizeItemId, RandomizeItemData) && LoadArmorDataRow(RandomizeItemId, RandomizeArmorData))
    {
        RandomizeItemName = RandomizeItemData.ItemName.ToString();
        const EItemClass RandomizeItemClass = RandomizeItemData.ItemClass;
        const FString RandomizeItemType = UEnum::GetValueAsString(RandomizeArmorData.ArmorType)
            .RightChop(FString(TEXT("EArmorType::")).Len());

        const TArray<FString> AssignedModifierIds =
            ItemModifierAssigner.AssignModifiers(RandomizeItemId, RandomizeItemClass, RandomizeItemType, ItemModifierAssigner.ModifierCount);

        CachedArmorStats.ImplicitModifiers.Empty();
        CachedArmorStats.PrefixModifiers.Empty();
        CachedArmorStats.SuffixModifiers.Empty();

        float GoldValueMultiplier = 1.f;
        for (const FString& ModifierId : AssignedModifierIds)
        {
            const FItemModifierStruct* ModifierRow = ItemModifierAssigner.GetModifierPool().FindByPredicate(
                [&ModifierId](const FItemModifierStruct& Modifier)
                {
                    return Modifier.ModifierId.ToString().Equals(ModifierId, ESearchCase::IgnoreCase);
                });

            int32 RolledValue = 0;
            EAffixType AffixType = EAffixType::Prefix;
            if (ModifierRow)
            {
                AffixType = ModifierRow->ModifierAffixType;
                if (ModifierRow->MinMaxRange.Num() >= 2)
                {
                    RolledValue = FMath::RandRange(ModifierRow->MinMaxRange[0], ModifierRow->MinMaxRange[1]);
                }
                else if (ModifierRow->MinMaxRange.Num() == 1)
                {
                    RolledValue = ModifierRow->MinMaxRange[0];
                }
            }

            switch (AffixType)
            {
            case EAffixType::Implicit: CachedArmorStats.ImplicitModifiers.Add(ModifierId, RolledValue); break;
            case EAffixType::Suffix:   CachedArmorStats.SuffixModifiers.Add(ModifierId, RolledValue);   break;
            case EAffixType::Prefix:
            default:                   CachedArmorStats.PrefixModifiers.Add(ModifierId, RolledValue);   break;
            }

            if (ModifierRow)
            {
                GoldValueMultiplier *= ModifierRow->GoldValueModifier;
            }

            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: assigned modifier %s (%s) value=%d"),
                *ModifierId, *UEnum::GetValueAsString(AffixType), RolledValue);
        }

        // ItemGoldValue is the item's base gold value scaled by every rolled modifier's GoldValueModifier.
        CachedArmorStats.ItemGoldValue = CachedArmorStats.ItemBaseGoldValue * GoldValueMultiplier;

        // List prefixes before suffixes in the summary text, regardless of roll order.
        ModifiersText = BuildGroupedModifiersText(ItemModifierAssigner.GetModifierPool(),
            CachedArmorStats.ImplicitModifiers, CachedArmorStats.PrefixModifiers, CachedArmorStats.SuffixModifiers);

        // Fold the rolled defense modifiers into the item's defense values.
        RecalculateArmorDefense();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Could not resolve class/type for '%s' to assign modifiers."), *RandomizeItemId);
    }

    SavedItemsManager.SaveItem(GetUUID().ToString(), CachedArmorStats);

    const FString DefenseSummary = FString::Printf(
        TEXT("PhysicalMitigation:%d-%d | Evade:%d-%d | Overshield:%d-%d"),
        CachedArmorStats.BaseDefense.BasePhysicalMitigation.X,
        CachedArmorStats.BaseDefense.BasePhysicalMitigation.Y,
        CachedArmorStats.BaseDefense.BaseEvade.X,
        CachedArmorStats.BaseDefense.BaseEvade.Y,
        CachedArmorStats.BaseDefense.BaseOvershield.X,
        CachedArmorStats.BaseDefense.BaseOvershield.Y);

    const FString Message = FString::Printf(
        TEXT("ItemHandler randomized armor stats:\nUUID:%s\nDefense:%s"),
        *CachedArmorStats.UUID.ToString(),
        *DefenseSummary);

    if (ModifiersText.IsEmpty())
    {
        ModifiersText = TEXT("Modifiers: none\n");
    }

    // Matches the layout CoreMenu uses for a selected stash item: name, then a "Base Defense:"
    // block, Gold Value, then the modifiers block.
    const FString ActiveItemText = FString::Printf(
        TEXT("%s\nBase Defense:\n%sGold Value: %.0f\n%s"),
        *RandomizeItemName,
        *BuildArmorDefenseLines(CachedArmorStats.BaseDefense),
        GetDisplayGoldValue(CachedArmorStats),
        *ModifiersText);

    if (BoundCoreMenu)
    {
        BoundCoreMenu->LogToScreen(Message);
        BoundCoreMenu->SetActiveItemText(ActiveItemText);
    }

    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

    return true;
}

void UItemHandler::RecalculateWeaponLocalDamage()
{
    // Local damage starts as the item's base damage, then the rolled modifiers adjust it:
    //   - Addition modifiers roll two independent values within their MinMaxRange and add the lower to
    //     the min damage and the higher to the max damage (each scaled by the DamageModifier weight),
    //     so the modifier contributes its own spread.
    //   - Multiplication modifiers then scale the result by (1 + rolled%) for any damage type whose
    //     ModifiedAttribute.DamageModifier entry is non-zero (the magnitude only has to be non-zero, it
    //     does not need to match).
    FWeaponBaseDamage BaseDamage;
    if (const FWeaponBaseDamage* FoundBase = CachedWeaponStats.WeaponDamage.Find(TEXT("BaseDamage")))
    {
        BaseDamage = *FoundBase;
    }

    // Pair every rolled modifier with its data-table row and the value it rolled.
    struct FRolledModifier { const FItemModifierStruct* Row = nullptr; int32 Value = 0; };
    TArray<FRolledModifier> RolledModifiers;
    auto GatherRolled = [this, &RolledModifiers](const TMap<FString, int32>& Modifiers)
    {
        for (const TPair<FString, int32>& Entry : Modifiers)
        {
            const FItemModifierStruct* Row = ItemModifierAssigner.GetModifierPool().FindByPredicate(
                [&Entry](const FItemModifierStruct& Modifier)
                {
                    return Modifier.ModifierId.ToString().Equals(Entry.Key, ESearchCase::IgnoreCase);
                });
            if (Row)
            {
                RolledModifiers.Add({ Row, Entry.Value });
            }
        }
    };
    GatherRolled(CachedWeaponStats.ImplicitModifiers);
    GatherRolled(CachedWeaponStats.PrefixModifiers);
    GatherRolled(CachedWeaponStats.SuffixModifiers);

    struct FDamageChannel
    {
        FIntPoint FWeaponBaseDamage::* BaseField;
        FIntPoint FWeaponLocalDamage::* LocalField;
        float FItemModifierDamageStruct::* ModifierField;
    };
    static const FDamageChannel DamageChannels[] = {
        { &FWeaponBaseDamage::BasePhysicalDamage, &FWeaponLocalDamage::LocalPhysicalDamage, &FItemModifierDamageStruct::PhysicalDamage },
        { &FWeaponBaseDamage::BaseFireDamage,     &FWeaponLocalDamage::LocalFireDamage,     &FItemModifierDamageStruct::FireDamage },
        { &FWeaponBaseDamage::BaseIceDamage,      &FWeaponLocalDamage::LocalIceDamage,      &FItemModifierDamageStruct::IceDamage },
        { &FWeaponBaseDamage::BaseElectricDamage, &FWeaponLocalDamage::LocalElectricDamage, &FItemModifierDamageStruct::ElectricDamage },
        { &FWeaponBaseDamage::BaseAbyssalDamage,   &FWeaponLocalDamage::LocalAbyssalDamage,   &FItemModifierDamageStruct::AbyssalDamage },
    };

    FWeaponLocalDamage LocalDamage;
    for (const FDamageChannel& Channel : DamageChannels)
    {
        const FIntPoint BaseRange = BaseDamage.*Channel.BaseField;
        double MinValue = BaseRange.X;
        double MaxValue = BaseRange.Y;

        // Flat additions from Addition-operator modifiers that touch this damage type.
        for (const FRolledModifier& Rolled : RolledModifiers)
        {
            if (Rolled.Row->ModifierOperator != EModifierOperator::Addition)
            {
                continue;
            }

            const float DamageWeight = Rolled.Row->ModifiedAttribute.DamageModifier.*Channel.ModifierField;
            if (DamageWeight == 0.f)
            {
                continue;
            }

            // Pick a value between the modifier's MinMaxRange bounds for each end of the damage spread.
            int32 LowRoll = Rolled.Value;
            int32 HighRoll = Rolled.Value;
            if (Rolled.Row->MinMaxRange.Num() >= 2)
            {
                const int32 RollA = FMath::RandRange(Rolled.Row->MinMaxRange[0], Rolled.Row->MinMaxRange[1]);
                const int32 RollB = FMath::RandRange(Rolled.Row->MinMaxRange[0], Rolled.Row->MinMaxRange[1]);
                LowRoll = FMath::Min(RollA, RollB);
                HighRoll = FMath::Max(RollA, RollB);
            }

            MinValue += static_cast<double>(LowRoll) * DamageWeight;
            MaxValue += static_cast<double>(HighRoll) * DamageWeight;
        }

        // Percentage scaling from Multiplication-operator modifiers that touch this damage type.
        for (const FRolledModifier& Rolled : RolledModifiers)
        {
            if (Rolled.Row->ModifierOperator != EModifierOperator::Multiplication)
            {
                continue;
            }

            if (Rolled.Row->ModifiedAttribute.DamageModifier.*Channel.ModifierField == 0.f)
            {
                continue;
            }

            const double Multiplier = 1.0 + (static_cast<double>(Rolled.Value) / 100.0);
            MinValue *= Multiplier;
            MaxValue *= Multiplier;
        }

        FIntPoint& LocalRange = LocalDamage.*Channel.LocalField;
        LocalRange.X = FMath::RoundToInt(MinValue);
        LocalRange.Y = FMath::RoundToInt(MaxValue);
    }

    CachedWeaponStats.WeaponLocalDamage.Empty();
    CachedWeaponStats.WeaponLocalDamage.Add(TEXT("LocalDamage"), LocalDamage);
}

void UItemHandler::RecalculateArmorDefense()
{
    // Defense starts from the item's base row (reloaded here since CachedArmorStats.BaseDefense
    // is itself overwritten below), then the rolled modifiers adjust it the same way weapon damage
    // modifiers do: Addition modifiers roll two independent values within their MinMaxRange and add
    // the lower to the min and the higher to the max (each scaled by the mitigation weight), and
    // Multiplication modifiers then scale the result by (1 + rolled%) for any channel whose
    // ModifiedAttribute.MigitationModifier entry is non-zero.
    FBaseDefense BaseDefenseValue;
    FBaseArmorStruct ArmorData;
    if (LoadArmorDataRow(CachedArmorStats.ItemId.ToString(), ArmorData))
    {
        BaseDefenseValue = ArmorData.BaseDefense;
    }

    // Pair every rolled modifier with its data-table row and the value it rolled.
    struct FRolledModifier { const FItemModifierStruct* Row = nullptr; int32 Value = 0; };
    TArray<FRolledModifier> RolledModifiers;
    auto GatherRolled = [this, &RolledModifiers](const TMap<FString, int32>& Modifiers)
    {
        for (const TPair<FString, int32>& Entry : Modifiers)
        {
            const FItemModifierStruct* Row = ItemModifierAssigner.GetModifierPool().FindByPredicate(
                [&Entry](const FItemModifierStruct& Modifier)
                {
                    return Modifier.ModifierId.ToString().Equals(Entry.Key, ESearchCase::IgnoreCase);
                });
            if (Row)
            {
                RolledModifiers.Add({ Row, Entry.Value });
            }
        }
    };
    GatherRolled(CachedArmorStats.ImplicitModifiers);
    GatherRolled(CachedArmorStats.PrefixModifiers);
    GatherRolled(CachedArmorStats.SuffixModifiers);

    struct FDefenseChannel
    {
        FIntPoint FBaseDefense::* Field;
        float FItemModifierMitigationStruct::* ModifierField;
    };
    static const FDefenseChannel DefenseChannels[] = {
        { &FBaseDefense::BasePhysicalMitigation, &FItemModifierMitigationStruct::PhysicalReduction },
        { &FBaseDefense::BaseEvade,               &FItemModifierMitigationStruct::Evade },
        { &FBaseDefense::BaseOvershield,          &FItemModifierMitigationStruct::Overshield },
    };

    FBaseDefense NewDefense{};
    for (const FDefenseChannel& Channel : DefenseChannels)
    {
        const FIntPoint BaseRange = BaseDefenseValue.*Channel.Field;
        double MinValue = BaseRange.X;
        double MaxValue = BaseRange.Y;

        // Flat additions from Addition-operator modifiers that touch this defense channel.
        for (const FRolledModifier& Rolled : RolledModifiers)
        {
            if (Rolled.Row->ModifierOperator != EModifierOperator::Addition)
            {
                continue;
            }

            const float MitigationWeight = Rolled.Row->ModifiedAttribute.MigitationModifier.*Channel.ModifierField;
            if (MitigationWeight == 0.f)
            {
                continue;
            }

            // Pick a value between the modifier's MinMaxRange bounds for each end of the defense spread.
            int32 LowRoll = Rolled.Value;
            int32 HighRoll = Rolled.Value;
            if (Rolled.Row->MinMaxRange.Num() >= 2)
            {
                const int32 RollA = FMath::RandRange(Rolled.Row->MinMaxRange[0], Rolled.Row->MinMaxRange[1]);
                const int32 RollB = FMath::RandRange(Rolled.Row->MinMaxRange[0], Rolled.Row->MinMaxRange[1]);
                LowRoll = FMath::Min(RollA, RollB);
                HighRoll = FMath::Max(RollA, RollB);
            }

            MinValue += static_cast<double>(LowRoll) * MitigationWeight;
            MaxValue += static_cast<double>(HighRoll) * MitigationWeight;
        }

        // Percentage scaling from Multiplication-operator modifiers that touch this defense channel.
        for (const FRolledModifier& Rolled : RolledModifiers)
        {
            if (Rolled.Row->ModifierOperator != EModifierOperator::Multiplication)
            {
                continue;
            }

            if (Rolled.Row->ModifiedAttribute.MigitationModifier.*Channel.ModifierField == 0.f)
            {
                continue;
            }

            const double Multiplier = 1.0 + (static_cast<double>(Rolled.Value) / 100.0);
            MinValue *= Multiplier;
            MaxValue *= Multiplier;
        }

        FIntPoint& OutRange = NewDefense.*Channel.Field;
        OutRange.X = FMath::RoundToInt(MinValue);
        OutRange.Y = FMath::RoundToInt(MaxValue);
    }

    CachedArmorStats.BaseDefense = NewDefense;
}

void UItemHandler::OnSaveItem(FString ItemId)
{
    if (ItemId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Received empty ItemId for save."));
        return;
    }

    FBaseItemStruct ItemData;
    if (!LoadItemDataRow(ItemId, ItemData))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No item found in BaseItem_DT for ItemId '%s'."), *ItemId);
        return;
    }

    FString Message;
    if (ItemData.ItemClass == EItemClass::Armor || ItemData.ItemClass == EItemClass::Shield)
    {
        LastSelectedItemClass = EItemClass::Armor;
        if (CachedArmorStats.ItemId.IsEmpty() || CachedArmorStats.ItemId.ToString() != ItemId)
        {
            CachedArmorStats = GetArmorStatsForItem(ItemId);
        }
        if (CachedArmorStats.ItemId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No cached armor stats available to save for ItemId '%s'."), *ItemId);
            return;
        }

        const FString UUID = GetUUID().ToString();
        SavedItemsManager.SaveItem(UUID, CachedArmorStats);
        Message = FString::Printf(TEXT("Saved armor item %s (UUID: %s)"), *ItemId, *UUID);
    }
    else
    {
        LastSelectedItemClass = EItemClass::Weapon;
        if (CachedWeaponStats.ItemId.IsEmpty() || CachedWeaponStats.ItemId.ToString() != ItemId)
        {
            CachedWeaponStats = GetWeaponStatsForItem(ItemId);
        }
        if (CachedWeaponStats.ItemId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No cached weapon stats available to save for ItemId '%s'."), *ItemId);
            return;
        }

        const FString UUID = GetUUID().ToString();
        SavedItemsManager.SaveItem(UUID, CachedWeaponStats);
        Message = FString::Printf(TEXT("Saved weapon item %s (UUID: %s)"), *ItemId, *UUID);
    }

    if (BoundCoreMenu)
    {
        BoundCoreMenu->LogToScreen(Message);
    }
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
}

void UItemHandler::OnSellButtonClicked(FString ItemId, FString ItemUUID)
{
    if (ItemUUID.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: No saved-item UUID to remove for sell of ItemId '%s'."), *ItemId);
        return;
    }

    // Resolve the gold value and let listeners (e.g. PlayerInventory adding it to PlayerGoldCount)
    // handle the sale before the item is removed below. Broadcast() calls bound listeners
    // synchronously, so every listener has returned by the time RemoveSavedItem runs.
    const float SoldGoldValue = GetItemGoldValue(ItemUUID);
    OnItemSoldEvent.Broadcast(ItemId, ItemUUID, SoldGoldValue);

    SavedItemsManager.RemoveSavedItem(ItemUUID);

    if (BoundCoreMenu)
    {
        BoundCoreMenu->ClearSelectedItemUUID();
    }

    const FString Message = FString::Printf(TEXT("Removed sold item %s (UUID: %s) from SavedItems.json"), *ItemId, *ItemUUID);
    if (BoundCoreMenu)
    {
        BoundCoreMenu->LogToScreen(Message);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
    }
}

void UItemHandler::OnResetGame()
{
    CachedWeaponStats = FItemWeaponStatsStruct();
    CachedArmorStats = FItemArmorStatsStruct();
    LastSelectedItemClass = EItemClass::Weapon;

    SavedItemsManager.ClearAllSavedItems();

    UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Reset - cleared cached item stats and SavedItems.json."));
}

float UItemHandler::GetItemGoldValue(const FString& ItemUUID) const
{
    const TSharedPtr<FJsonObject> FoundItem = SavedItemsManager.GetSavedItemJson(ItemUUID);
    if (!FoundItem.IsValid())
    {
        return 0.f;
    }

    double ItemGoldValue = 0.0;
    double ItemBaseGoldValue = 0.0;
    FoundItem->TryGetNumberField(TEXT("itemGoldValue"), ItemGoldValue);
    FoundItem->TryGetNumberField(TEXT("itemBaseGoldValue"), ItemBaseGoldValue);
    return static_cast<float>(ItemGoldValue > 0.0 ? ItemGoldValue : ItemBaseGoldValue);
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

bool UItemHandler::LoadAllItemModifierRows(TArray<FItemModifierStruct>& OutModifiers) const
{
    OutModifiers.Reset();

    const FString DataTablePath = TEXT("/Game/ItemData/ItemModifier_DT.ItemModifier_DT");
    UDataTable* ModifierDataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!ModifierDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemHandler: Failed to load modifier data table at %s."), *DataTablePath);
        return false;
    }

    for (const FName& RowName : ModifierDataTable->GetRowNames())
    {
        if (FItemModifierStruct* ModifierRow = ModifierDataTable->FindRow<FItemModifierStruct>(RowName, TEXT("ItemHandler::LoadAllItemModifierRows"), true))
        {
            OutModifiers.Add(*ModifierRow);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Loaded %d modifiers from ItemModifier_DT."), OutModifiers.Num());
    return OutModifiers.Num() > 0;
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
        WeaponData.WeaponBaseDamage.BasePhysicalDamage.X,
        WeaponData.WeaponBaseDamage.BasePhysicalDamage.Y);

    UE_LOG(LogTemp, Warning, TEXT("ItemHandler loaded weapon table data:\n%s"), *WeaponInfo);
}

void UItemHandler::LogArmorData(const FBaseArmorStruct& ArmorData) const
{
    const FString ArmorInfo = FString::Printf(
        TEXT("ArmorType: %s\nArmorSlot: %s\nBasePhysicalMitigation: %d-%d\nBaseEvade: %d-%d\nBaseOvershield: %d-%d"),
        *UEnum::GetValueAsString(ArmorData.ArmorType),
        *UEnum::GetValueAsString(ArmorData.ArmorSlot),
        ArmorData.BaseDefense.BasePhysicalMitigation.X,
        ArmorData.BaseDefense.BasePhysicalMitigation.Y,
        ArmorData.BaseDefense.BaseEvade.X,
        ArmorData.BaseDefense.BaseEvade.Y,
        ArmorData.BaseDefense.BaseOvershield.X,
        ArmorData.BaseDefense.BaseOvershield.Y);

    UE_LOG(LogTemp, Warning, TEXT("ItemHandler loaded armor table data:\n%s"), *ArmorInfo);
}
