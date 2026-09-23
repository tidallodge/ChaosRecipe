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

	// Picks one entry at random from a currency's ModifiersAffectedWeightedRange (e.g. [3,4,4,5,5,6]
	// weights 4 and 5 as twice as likely as 3 or 6, matching how the pre-currency full reroll picked its
	// modifier count). Falls back to DefaultIfEmpty when the range is empty.
	int32 PickWeightedCount(const TArray<int32>& WeightedRange, int32 DefaultIfEmpty)
	{
		if (WeightedRange.Num() == 0)
		{
			return DefaultIfEmpty;
		}

		return WeightedRange[FMath::RandRange(0, WeightedRange.Num() - 1)];
	}

	// Fisher-Yates shuffle in place, so callers can take the first N entries as a random subset.
	void ShuffleIds(TArray<FString>& Ids)
	{
		for (int32 i = Ids.Num() - 1; i > 0; --i)
		{
			const int32 j = FMath::RandRange(0, i);
			Ids.Swap(i, j);
		}
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

void UItemHandler::OnRandomizeItem(FString CurrencyId)
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

    if (BoundPlayerInventory && !CurrencyId.IsEmpty() && !BoundPlayerInventory->CanAffordCurrency(CurrencyId, 1))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: Player does not have currency '%s' to randomize with."), *CurrencyId);
        if (BoundCoreMenu)
        {
            BoundCoreMenu->SetWarningText(TEXT("You don't have that currency."));
        }
        return;
    }

    bool bDidRandomize = false;
    if (LastSelectedItemClass == EItemClass::Weapon)
    {
        bDidRandomize = RandomizeWeaponItem(CurrencyId);
    }
    else if (LastSelectedItemClass == EItemClass::Armor)
    {
        bDidRandomize = RandomizeArmorItem(CurrencyId);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ItemHandler: Randomizing item class '%s' is not supported yet."),
            *UEnum::GetValueAsString(LastSelectedItemClass));
    }

    if (bDidRandomize)
    {
        OnItemRandomizedEvent.Broadcast(RandomizeItemGoldCost, CurrencyId);
    }
}

bool UItemHandler::ApplyCurrencyToItemModifiers(const FCurrencyStruct& Currency, const FString& ItemId, EItemClass ItemClass,
    const FString& ItemType, TMap<FString, int32>& ImplicitModifiers, TMap<FString, int32>& PrefixModifiers,
    TMap<FString, int32>& SuffixModifiers)
{
    const FAffectedAffixes& Affected = Currency.AffectedAffixes;
    const TArray<FItemModifierStruct> FullPool = ItemModifierAssigner.GetModifierPool();

    auto FindRow = [&FullPool](const FString& Id) -> const FItemModifierStruct*
    {
        return FullPool.FindByPredicate([&Id](const FItemModifierStruct& Modifier)
        {
            return Modifier.ModifierId.ToString().Equals(Id, ESearchCase::IgnoreCase);
        });
    };

    auto GetAllExistingIds = [&]() -> TArray<FString>
    {
        TArray<FString> Ids;
        ImplicitModifiers.GetKeys(Ids);
        TArray<FString> PrefixIds;
        PrefixModifiers.GetKeys(PrefixIds);
        Ids.Append(PrefixIds);
        TArray<FString> SuffixIds;
        SuffixModifiers.GetKeys(SuffixIds);
        Ids.Append(SuffixIds);
        return Ids;
    };

    auto GetEligibleExistingIds = [&]() -> TArray<FString>
    {
        TArray<FString> Ids;
        if (Affected.Implicits)
        {
            TArray<FString> ImplicitIds;
            ImplicitModifiers.GetKeys(ImplicitIds);
            Ids.Append(ImplicitIds);
        }
        if (Affected.Prefixes)
        {
            TArray<FString> PrefixIds;
            PrefixModifiers.GetKeys(PrefixIds);
            Ids.Append(PrefixIds);
        }
        if (Affected.Suffixes)
        {
            TArray<FString> SuffixIds;
            SuffixModifiers.GetKeys(SuffixIds);
            Ids.Append(SuffixIds);
        }
        return Ids;
    };

    auto RemoveFromMaps = [&](const FString& Id)
    {
        ImplicitModifiers.Remove(Id);
        PrefixModifiers.Remove(Id);
        SuffixModifiers.Remove(Id);
    };

    auto RollValueForRow = [](const FItemModifierStruct* Row) -> int32
    {
        if (!Row) return 0;
        if (Row->MinMaxRange.Num() >= 2) return FMath::RandRange(Row->MinMaxRange[0], Row->MinMaxRange[1]);
        if (Row->MinMaxRange.Num() == 1) return Row->MinMaxRange[0];
        return 0;
    };

    auto AddModifierToMaps = [&](const FString& Id)
    {
        const FItemModifierStruct* Row = FindRow(Id);
        const int32 RolledValue = RollValueForRow(Row);
        const EAffixType AffixType = Row ? Row->ModifierAffixType : EAffixType::Prefix;
        switch (AffixType)
        {
        case EAffixType::Implicit: ImplicitModifiers.Add(Id, RolledValue); break;
        case EAffixType::Suffix:   SuffixModifiers.Add(Id, RolledValue);   break;
        case EAffixType::Prefix:
        default:                   PrefixModifiers.Add(Id, RolledValue);  break;
        }
    };

    // Rolls AddCount brand-new modifiers via the existing ModifierAssigner, restricted to whatever this
    // currency's AffectedAffixes/CurrencyModifierTags allow and to whatever ValidateModifierPool says
    // doesn't collide with a bucket already on the item (both already-public ModifierAssigner
    // capabilities - no new rolling logic needed here). AssignModifiers has no notion of modifiers
    // already on the item, so this temporarily swaps the assigner down to a pre-filtered pool and always
    // restores the full pool afterward, so later id lookups (display text, gold value, local damage)
    // still resolve every modifier, old and new.
    auto RollAndAddNewModifiers = [&](const TArray<FString>& ExistingIds, int32 AddCount) -> TArray<FString>
    {
        if (AddCount <= 0)
        {
            return TArray<FString>();
        }

        int32 PrefixCount = 0;
        int32 SuffixCount = 0;
        for (const FString& Id : ExistingIds)
        {
            if (const FItemModifierStruct* Row = FindRow(Id))
            {
                if (Row->ModifierAffixType == EAffixType::Prefix) ++PrefixCount;
                else if (Row->ModifierAffixType == EAffixType::Suffix) ++SuffixCount;
            }
        }

        const bool bAnyTagRequired = ModifierAssigner::HasAnyTagSet(Currency.CurrencyModifierTags);
        const TArray<FString> ValidIds = ItemModifierAssigner.ValidateModifierPool(FullPool, ExistingIds);

        TArray<FItemModifierStruct> RestrictedPool;
        for (const FItemModifierStruct& Modifier : FullPool)
        {
            const FString ModifierId = Modifier.ModifierId.ToString();
            const bool bBucketValid = ValidIds.ContainsByPredicate(
                [&ModifierId](const FString& ValidId) { return ModifierId.Equals(ValidId, ESearchCase::IgnoreCase); });
            if (!bBucketValid)
            {
                continue;
            }

            const bool bAffixAllowed =
                (Modifier.ModifierAffixType == EAffixType::Prefix && Affected.Prefixes && PrefixCount < 3) ||
                (Modifier.ModifierAffixType == EAffixType::Suffix && Affected.Suffixes && SuffixCount < 3) ||
                (Modifier.ModifierAffixType == EAffixType::Implicit && Affected.Implicits);
            if (!bAffixAllowed)
            {
                continue;
            }

            if (bAnyTagRequired && !ModifierAssigner::ModifierTagsOverlap(Modifier.ModifierTags, Currency.CurrencyModifierTags))
            {
                continue;
            }

            RestrictedPool.Add(Modifier);
        }

        ItemModifierAssigner.SetModifierPool(RestrictedPool);
        const TArray<FString> RolledIds = ItemModifierAssigner.AssignModifiers(ItemId, ItemClass, ItemType, AddCount);
        ItemModifierAssigner.SetModifierPool(FullPool);

        // AssignModifiers enforces its own 3-prefix/3-suffix cap, but starts counting from 0 - it has no
        // notion of ExistingIds. Clamp here against the true existing counts so the combined (existing +
        // newly rolled) total never exceeds the cap even when both affix types are being added to at once.
        int32 RemainingPrefixRoom = FMath::Max(0, 3 - PrefixCount);
        int32 RemainingSuffixRoom = FMath::Max(0, 3 - SuffixCount);
        TArray<FString> NewIds;
        for (const FString& RolledId : RolledIds)
        {
            const FItemModifierStruct* Row = FindRow(RolledId);
            const EAffixType AffixType = Row ? Row->ModifierAffixType : EAffixType::Implicit;
            if (AffixType == EAffixType::Prefix)
            {
                if (RemainingPrefixRoom <= 0) continue;
                --RemainingPrefixRoom;
            }
            else if (AffixType == EAffixType::Suffix)
            {
                if (RemainingSuffixRoom <= 0) continue;
                --RemainingSuffixRoom;
            }
            NewIds.Add(RolledId);
        }

        return NewIds;
    };

    switch (Currency.ModifierModificationType)
    {
    case EModifierModificationType::Add:
    {
        const int32 AddCount = PickWeightedCount(Currency.ModifiersAffectedWeightedRange, 1);
        const TArray<FString> NewIds = RollAndAddNewModifiers(GetAllExistingIds(), AddCount);
        if (NewIds.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: currency %s had no eligible modifiers to add."), *Currency.CurrencyId.ToString());
            if (BoundCoreMenu) BoundCoreMenu->SetWarningText(TEXT("No room for additional modifiers."));
            return false;
        }

        for (const FString& NewId : NewIds)
        {
            AddModifierToMaps(NewId);
        }
        return true;
    }
    case EModifierModificationType::Remove:
    {
        TArray<FString> EligibleIds = GetEligibleExistingIds();
        if (EligibleIds.Num() == 0)
        {
            if (BoundCoreMenu) BoundCoreMenu->SetWarningText(TEXT("No eligible modifiers to remove."));
            return false;
        }

        const int32 RemoveCount = FMath::Min(PickWeightedCount(Currency.ModifiersAffectedWeightedRange, 1), EligibleIds.Num());
        ShuffleIds(EligibleIds);
        for (int32 i = 0; i < RemoveCount; ++i)
        {
            RemoveFromMaps(EligibleIds[i]);
        }
        return true;
    }
    case EModifierModificationType::RandomizeMods:
    {
        TArray<FString> EligibleIds = GetEligibleExistingIds();
        if (EligibleIds.Num() == 0)
        {
            if (BoundCoreMenu) BoundCoreMenu->SetWarningText(TEXT("No eligible modifiers to reroll."));
            return false;
        }

        // Wipes every eligible modifier (not just a subset) and rolls a brand new set sized from the
        // currency's weighted range - independent of how many were just removed, so e.g. a 3-prefix item
        // can land anywhere from 1 to 3 new prefixes.
        for (const FString& Id : EligibleIds)
        {
            RemoveFromMaps(Id);
        }

        const int32 AddCount = PickWeightedCount(Currency.ModifiersAffectedWeightedRange, 1);
        const TArray<FString> NewIds = RollAndAddNewModifiers(GetAllExistingIds(), AddCount);
        for (const FString& NewId : NewIds)
        {
            AddModifierToMaps(NewId);
        }
        return true;
    }
    case EModifierModificationType::RandomizeValues:
    {
        TArray<FString> EligibleIds = GetEligibleExistingIds();
        if (EligibleIds.Num() == 0)
        {
            if (BoundCoreMenu) BoundCoreMenu->SetWarningText(TEXT("No eligible modifiers to reroll."));
            return false;
        }

        TArray<FString> TargetIds;
        if (Currency.ModifiersAffectedWeightedRange.Num() == 0)
        {
            // Empty range = affect every eligible modifier's value (e.g. Divine/Bless-style currencies).
            TargetIds = EligibleIds;
        }
        else
        {
            const int32 Count = FMath::Min(PickWeightedCount(Currency.ModifiersAffectedWeightedRange, 1), EligibleIds.Num());
            ShuffleIds(EligibleIds);
            for (int32 i = 0; i < Count; ++i)
            {
                TargetIds.Add(EligibleIds[i]);
            }
        }

        for (const FString& TargetId : TargetIds)
        {
            const int32 RolledValue = RollValueForRow(FindRow(TargetId));
            if (ImplicitModifiers.Contains(TargetId))     ImplicitModifiers.Add(TargetId, RolledValue);
            else if (PrefixModifiers.Contains(TargetId))  PrefixModifiers.Add(TargetId, RolledValue);
            else if (SuffixModifiers.Contains(TargetId))  SuffixModifiers.Add(TargetId, RolledValue);
        }
        return true;
    }
    case EModifierModificationType::RandomizeTiers:
    case EModifierModificationType::Other:
    default:
        UE_LOG(LogTemp, Warning, TEXT("ItemHandler: currency %s uses an unsupported ModifierModificationType (%s); no changes made."),
            *Currency.CurrencyId.ToString(), *UEnum::GetValueAsString(Currency.ModifierModificationType));
        if (BoundCoreMenu) BoundCoreMenu->SetWarningText(TEXT("This currency isn't supported yet."));
        return false;
    }
}

float UItemHandler::ComputeModifiedGoldValue(float ItemBaseGoldValue, const TMap<FString, int32>& ImplicitModifiers,
    const TMap<FString, int32>& PrefixModifiers, const TMap<FString, int32>& SuffixModifiers) const
{
    float GoldValueMultiplier = 1.f;

    auto ApplyGroup = [this, &GoldValueMultiplier](const TMap<FString, int32>& Modifiers)
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
                GoldValueMultiplier *= Row->GoldValueModifier;
            }
        }
    };

    ApplyGroup(ImplicitModifiers);
    ApplyGroup(PrefixModifiers);
    ApplyGroup(SuffixModifiers);

    return ItemBaseGoldValue * GoldValueMultiplier;
}

bool UItemHandler::RandomizeWeaponItem(const FString& CurrencyId)
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

    // Resolve this item's class and type, then either do a full reroll (no currency) or apply the
    // chosen currency's modifier-modification rules to whatever is already on the item.
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

        if (CurrencyId.IsEmpty())
        {
            // No currency selected: full reroll, matching the original (pre-currency) behavior - wipe
            // back to base stats and roll an entirely new set of modifiers.
            CachedWeaponStats = GetWeaponStatsForItem(RandomizeItemId);

            // Roll how many modifiers this item gets: 3-6, weighted so 4 and 5 are the common outcomes.
            static const int32 ModifierCountChoices[] = { 3, 4, 4, 5, 5, 6 };
            ItemModifierAssigner.ModifierCount = ModifierCountChoices[FMath::RandRange(0, UE_ARRAY_COUNT(ModifierCountChoices) - 1)];
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: rolled ModifierCount=%d for this randomize."), ItemModifierAssigner.ModifierCount);

            const TArray<FString> AssignedModifierIds =
                ItemModifierAssigner.AssignModifiers(RandomizeItemId, RandomizeItemClass, RandomizeItemType, ItemModifierAssigner.ModifierCount);

            CachedWeaponStats.ImplicitModifiers.Empty();
            CachedWeaponStats.PrefixModifiers.Empty();
            CachedWeaponStats.SuffixModifiers.Empty();

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

                UE_LOG(LogTemp, Warning, TEXT("ItemHandler: assigned modifier %s (%s) value=%d"),
                    *ModifierId, *UEnum::GetValueAsString(AffixType), RolledValue);
            }
        }
        else
        {
            FCurrencyStruct Currency;
            if (!CurrencyManagerInstance.LoadCurrencyDataRow(CurrencyId, Currency))
            {
                UE_LOG(LogTemp, Warning, TEXT("ItemHandler: unknown currency '%s'."), *CurrencyId);
                if (BoundCoreMenu)
                {
                    BoundCoreMenu->SetWarningText(TEXT("Unknown currency."));
                }
                return false;
            }

            const bool bModifiersChanged = ApplyCurrencyToItemModifiers(Currency, RandomizeItemId, RandomizeItemClass, RandomizeItemType,
                CachedWeaponStats.ImplicitModifiers, CachedWeaponStats.PrefixModifiers, CachedWeaponStats.SuffixModifiers);

            if (!bModifiersChanged)
            {
                return false;
            }
        }

        // ItemGoldValue is the item's base gold value scaled by every currently-assigned modifier's GoldValueModifier.
        CachedWeaponStats.ItemGoldValue = ComputeModifiedGoldValue(CachedWeaponStats.ItemBaseGoldValue,
            CachedWeaponStats.ImplicitModifiers, CachedWeaponStats.PrefixModifiers, CachedWeaponStats.SuffixModifiers);

        // List prefixes before suffixes in the summary text, regardless of roll order.
        ModifiersText = BuildGroupedModifiersText(ItemModifierAssigner.GetModifierPool(),
            CachedWeaponStats.ImplicitModifiers, CachedWeaponStats.PrefixModifiers, CachedWeaponStats.SuffixModifiers);

        // Fold the rolled damage/attack-rate modifiers into the item's local damage and attack rate.
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

bool UItemHandler::RandomizeArmorItem(const FString& CurrencyId)
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

    // Resolve this item's class and type, then either do a full reroll (no currency) or apply the
    // chosen currency's modifier-modification rules to whatever is already on the item.
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

        if (CurrencyId.IsEmpty())
        {
            // No currency selected: full reroll, matching the original (pre-currency) behavior - wipe
            // back to base stats and roll an entirely new set of modifiers.
            CachedArmorStats = GetArmorStatsForItem(RandomizeItemId);

            // Roll how many modifiers this item gets: 3-6, weighted so 4 and 5 are the common outcomes.
            static const int32 ModifierCountChoices[] = { 3, 4, 4, 5, 5, 6 };
            ItemModifierAssigner.ModifierCount = ModifierCountChoices[FMath::RandRange(0, UE_ARRAY_COUNT(ModifierCountChoices) - 1)];
            UE_LOG(LogTemp, Warning, TEXT("ItemHandler: rolled ModifierCount=%d for this randomize."), ItemModifierAssigner.ModifierCount);

            const TArray<FString> AssignedModifierIds =
                ItemModifierAssigner.AssignModifiers(RandomizeItemId, RandomizeItemClass, RandomizeItemType, ItemModifierAssigner.ModifierCount);

            CachedArmorStats.ImplicitModifiers.Empty();
            CachedArmorStats.PrefixModifiers.Empty();
            CachedArmorStats.SuffixModifiers.Empty();

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

                UE_LOG(LogTemp, Warning, TEXT("ItemHandler: assigned modifier %s (%s) value=%d"),
                    *ModifierId, *UEnum::GetValueAsString(AffixType), RolledValue);
            }
        }
        else
        {
            FCurrencyStruct Currency;
            if (!CurrencyManagerInstance.LoadCurrencyDataRow(CurrencyId, Currency))
            {
                UE_LOG(LogTemp, Warning, TEXT("ItemHandler: unknown currency '%s'."), *CurrencyId);
                if (BoundCoreMenu)
                {
                    BoundCoreMenu->SetWarningText(TEXT("Unknown currency."));
                }
                return false;
            }

            const bool bModifiersChanged = ApplyCurrencyToItemModifiers(Currency, RandomizeItemId, RandomizeItemClass, RandomizeItemType,
                CachedArmorStats.ImplicitModifiers, CachedArmorStats.PrefixModifiers, CachedArmorStats.SuffixModifiers);

            if (!bModifiersChanged)
            {
                return false;
            }
        }

        // ItemGoldValue is the item's base gold value scaled by every currently-assigned modifier's GoldValueModifier.
        CachedArmorStats.ItemGoldValue = ComputeModifiedGoldValue(CachedArmorStats.ItemBaseGoldValue,
            CachedArmorStats.ImplicitModifiers, CachedArmorStats.PrefixModifiers, CachedArmorStats.SuffixModifiers);

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

    // AttackRate is derived from scratch here (base rate scaled by every currently-assigned
    // WeaponAttackRate modifier) rather than accumulated incrementally at roll time, so it stays correct
    // across partial modifier changes (currency add/remove/reroll), not just a full reroll from base.
    float BaseAttackRate = 0.f;
    FBaseWeaponStruct WeaponData;
    if (LoadWeaponDataRow(CachedWeaponStats.ItemId.ToString(), WeaponData))
    {
        BaseAttackRate = WeaponData.WeaponBaseAttackRate;
    }

    float AttackRateMultiplier = 1.f;
    for (const FRolledModifier& Rolled : RolledModifiers)
    {
        if (Rolled.Row->ModifiedAttribute.WeaponAttackRate != 0.f)
        {
            AttackRateMultiplier *= 1.f + (Rolled.Value / 100.f);
        }
    }
    CachedWeaponStats.AttackRate = BaseAttackRate * AttackRateMultiplier;

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
