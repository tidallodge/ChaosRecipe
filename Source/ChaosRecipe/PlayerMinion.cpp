// PlayerMinion
/** Purpose: A player-owned combatant built from a minion class (MinionClass_DT) and level
    Equips weapons and armor from the player's stash, which modify its stats and attacks
    Uses attacks/spells on anything that implements ICombatTarget, and takes hits through it too
*/

#include "PlayerMinion.h"
#include "CombatAbilityStruct.h"
#include "ItemInstanceManager.h"
#include "JsonObjectConverter.h"
#include "DataTableHelpers.h"

namespace
{
	const TCHAR* MinionClassDataTablePath = TEXT("/Game/MinionData/MinionClass_DT.MinionClass_DT");
	const TCHAR* BaseWeaponDataTablePath = TEXT("/Game/ItemData/BaseWeapon_DT.BaseWeapon_DT");
	const TCHAR* BaseArmorDataTablePath = TEXT("/Game/ItemData/BaseArmor_DT.BaseArmor_DT");
	const TCHAR* ItemModifierDataTablePath = TEXT("/Game/ItemData/ItemModifier_DT.ItemModifier_DT");

	// An equipped item's modifier row paired with the value it rolled.
	struct FRolledModifier { const FItemModifierStruct* Row = nullptr; int32 Value = 0; };

	// Applies every modifier whose GetWeight is non-zero to BaseValue: Addition/GlobalMod modifiers add
	// Value * weight first, then Multiplication modifiers scale by (1 + Value%), the same flat-then-percent
	// order ItemHandler uses when it bakes local modifiers into an item.
	float ApplyModifiersToStat(float BaseValue, const TArray<FRolledModifier>& Modifiers,
		TFunctionRef<float(const FItemModifierAffectedAttributes&)> GetWeight)
	{
		float FlatValue = BaseValue;
		float Multiplier = 1.f;
		for (const FRolledModifier& Rolled : Modifiers)
		{
			const float Weight = GetWeight(Rolled.Row->ModifiedAttribute);
			if (Weight == 0.f)
			{
				continue;
			}

			switch (Rolled.Row->ModifierOperator)
			{
			case EModifierOperator::Addition:
			case EModifierOperator::GlobalMod:      FlatValue += Rolled.Value * Weight;       break;
			case EModifierOperator::Multiplication: Multiplier *= 1.f + (Rolled.Value / 100.f); break;
			default:                                                                          break;
			}
		}

		return FlatValue * Multiplier;
	}
}

bool UPlayerMinion::InitializeMinion(EMinionClass InMinionClass, int32 InLevel)
{
	FMinionClassStruct LoadedClassData;
	const bool bFoundClass = DataTableHelpers::FindRow(MinionClassDataTablePath,
		[InMinionClass](const FMinionClassStruct& Row) { return Row.MinionClass == InMinionClass; }, LoadedClassData);
	if (!bFoundClass)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerMinion: No MinionClass_DT row for %s."), *UEnum::GetValueAsString(InMinionClass));
		return false;
	}

	ClassData = LoadedClassData;
	Level = FMath::Max(1, InLevel);

	if (ClassData.Damage.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: %s has no Damage set in MinionClass_DT, so it deals no damage without a weapon."),
			*ClassData.ClassName.ToString());
	}

	RecalculateStats();
	RestoreToFull();

	UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Initialized %s at level %d (Health=%d, Str=%d, Int=%d, Dex=%d)"),
		*ClassData.ClassName.ToString(), Level, TotalAttributes.Health, TotalAttributes.Strength,
		TotalAttributes.Intelligence, TotalAttributes.Dexterity);
	return true;
}

void UPlayerMinion::SetLevel(int32 NewLevel)
{
	Level = FMath::Max(1, NewLevel);
	RecalculateStats();
}

void UPlayerMinion::BindToItemHandlerEvents(UItemHandler* ItemHandler)
{
	if (!ItemHandler)
	{
		UE_LOG(LogTemp, Error, TEXT("no ItemHandler for PlayerMinion"));
		return;
	}

	ItemHandler->OnItemSoldEvent.AddDynamic(this, &UPlayerMinion::HandleItemSold);
	ItemHandler->OnItemRandomizedEvent.AddDynamic(this, &UPlayerMinion::HandleItemRandomized);
}

void UPlayerMinion::HandleItemSold(FString ItemId, FString ItemUUID, float GoldValue)
{
	if (UnequipItem(ItemUUID))
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Unequipped %s (UUID: %s) because it was sold."), *ItemId, *ItemUUID);
	}
}

void UPlayerMinion::HandleItemRandomized(int32 GoldCost, FString CurrencyId)
{
	// OnItemRandomizedEvent doesn't say which item was rerolled, but it's only broadcast after the reroll
	// has been saved, so re-reading everything equipped picks up whichever one it was.
	RefreshEquippedItems();
}

bool UPlayerMinion::EquipItem(const FString& ItemUUID)
{
	// Read-only: this instance never saves, so it can't clobber ItemHandler's own UItemInstanceManager.
	UItemInstanceManager StashManager;
	const TSharedPtr<FJsonObject> ItemJson = StashManager.GetSavedItemJson(ItemUUID);
	if (!ItemJson.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: No stash item with UUID '%s' to equip."), *ItemUUID);
		return false;
	}

	FString ItemType;
	ItemJson->TryGetStringField(TEXT("ItemType"), ItemType);

	if (ItemType == TEXT("Weapon"))
	{
		FItemWeaponStatsStruct WeaponStats;
		if (!FJsonObjectConverter::JsonObjectToUStruct(ItemJson.ToSharedRef(), &WeaponStats))
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Failed to read weapon stats for UUID '%s'."), *ItemUUID);
			return false;
		}

		WeaponStats.UUID = FText::FromString(ItemUUID);
		return EquipWeapon(WeaponStats);
	}

	if (ItemType == TEXT("Armor"))
	{
		FItemArmorStatsStruct ArmorStats;
		if (!FJsonObjectConverter::JsonObjectToUStruct(ItemJson.ToSharedRef(), &ArmorStats))
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Failed to read armor stats for UUID '%s'."), *ItemUUID);
			return false;
		}

		ArmorStats.UUID = FText::FromString(ItemUUID);
		return EquipArmor(ArmorStats);
	}

	UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Can't equip UUID '%s' with unsupported ItemType '%s'."), *ItemUUID, *ItemType);
	return false;
}

bool UPlayerMinion::EquipWeapon(const FItemWeaponStatsStruct& WeaponStats)
{
	const FString ItemId = WeaponStats.ItemId.ToString();
	FBaseWeaponStruct WeaponBase;
	const bool bFoundWeapon = DataTableHelpers::FindRow(BaseWeaponDataTablePath,
		[&ItemId](const FBaseWeaponStruct& Row) { return Row.ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase); }, WeaponBase);
	if (!bFoundWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: No BaseWeapon_DT row for '%s'; can't equip it."), *ItemId);
		return false;
	}

	if (WeaponBase.WeaponSlot == EWeaponSlot::TwoHand && EquippedArmor.Remove(EArmorSlot::OffHand) > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Unequipped off-hand to make room for two-handed %s."), *ItemId);
	}

	EquippedWeapon = WeaponStats;
	EquippedWeaponBase = WeaponBase;
	RecalculateStats();

	UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Equipped weapon %s (UUID: %s)"), *ItemId, *WeaponStats.UUID.ToString());
	return true;
}

bool UPlayerMinion::EquipArmor(const FItemArmorStatsStruct& ArmorStats)
{
	const FString ItemId = ArmorStats.ItemId.ToString();
	FBaseArmorStruct ArmorBase;
	const bool bFoundArmor = DataTableHelpers::FindRow(BaseArmorDataTablePath,
		[&ItemId](const FBaseArmorStruct& Row) { return Row.ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase); }, ArmorBase);
	if (!bFoundArmor)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: No BaseArmor_DT row for '%s'; can't equip it."), *ItemId);
		return false;
	}

	if (ArmorBase.ArmorSlot == EArmorSlot::OffHand && HasWeaponEquipped() && EquippedWeaponBase.WeaponSlot == EWeaponSlot::TwoHand)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Unequipped two-handed %s to make room for off-hand %s."),
			*EquippedWeapon.ItemId.ToString(), *ItemId);
		ClearEquippedWeapon();
	}

	EquippedArmor.Add(ArmorBase.ArmorSlot, ArmorStats);
	RecalculateStats();

	UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Equipped %s armor %s (UUID: %s)"),
		*UEnum::GetValueAsString(ArmorBase.ArmorSlot), *ItemId, *ArmorStats.UUID.ToString());
	return true;
}

bool UPlayerMinion::UnequipItem(const FString& ItemUUID)
{
	bool bUnequipped = false;
	if (HasWeaponEquipped() && EquippedWeapon.UUID.ToString() == ItemUUID)
	{
		ClearEquippedWeapon();
		bUnequipped = true;
	}

	for (auto It = EquippedArmor.CreateIterator(); It; ++It)
	{
		if (It.Value().UUID.ToString() == ItemUUID)
		{
			It.RemoveCurrent();
			bUnequipped = true;
		}
	}

	if (bUnequipped)
	{
		RecalculateStats();
	}
	return bUnequipped;
}

void UPlayerMinion::UnequipAll()
{
	ClearEquippedWeapon();
	EquippedArmor.Empty();
	RecalculateStats();
}

void UPlayerMinion::RefreshEquippedItems()
{
	for (const FString& ItemUUID : GetEquippedItemUUIDs())
	{
		if (!EquipItem(ItemUUID))
		{
			UnequipItem(ItemUUID);
		}
	}
}

bool UPlayerMinion::IsItemEquipped(const FString& ItemUUID) const
{
	return GetEquippedItemUUIDs().Contains(ItemUUID);
}

TArray<FString> UPlayerMinion::GetEquippedItemUUIDs() const
{
	TArray<FString> ItemUUIDs;
	if (HasWeaponEquipped())
	{
		ItemUUIDs.Add(EquippedWeapon.UUID.ToString());
	}

	for (const TPair<EArmorSlot, FItemArmorStatsStruct>& Entry : EquippedArmor)
	{
		ItemUUIDs.Add(Entry.Value.UUID.ToString());
	}
	return ItemUUIDs;
}

void UPlayerMinion::ClearEquippedWeapon()
{
	EquippedWeapon = FItemWeaponStatsStruct();
	EquippedWeaponBase = FBaseWeaponStruct();
}

FCombatHitResult UPlayerMinion::UseAbility(const FCombatAbilityStruct& Ability, ICombatTarget* Target)
{
	if (IsDefeated() || !Target || Target->IsDefeated())
	{
		return FCombatHitResult();
	}

	if (Level < Ability.RequiredLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Level %d is too low to use %s (requires %d)."),
			Level, *Ability.AbilityName.ToString(), Ability.RequiredLevel);
		return FCombatHitResult();
	}

	const FCombatHit Hit = CombatMath::BuildHit(Ability, Offense);
	return Target->ApplyHit(Hit);
}

FCombatHitResult UPlayerMinion::ApplyHit(const FCombatHit& Hit)
{
	if (IsDefeated())
	{
		return FCombatHitResult();
	}

	const FCombatHitResult Result = CombatMath::ResolveHit(Hit, Defense, CurrentHealth, CurrentOvershield);

	UE_LOG(LogTemp, Warning, TEXT("PlayerMinion: Hit for %.1f health / %.1f overshield%s%s (Health=%.1f/%.0f)"),
		Result.HealthDamage, Result.OvershieldDamage, Result.bIsCritical ? TEXT(", critical") : TEXT(""),
		Result.bEvaded ? TEXT(", evaded") : TEXT(""), CurrentHealth, GetMaxHealth());

	if (Result.HealthDamage > 0.f || Result.OvershieldDamage > 0.f)
	{
		BroadcastHealthChanged();
	}

	if (Result.bKilledTarget)
	{
		OnDiedEvent.Broadcast();
	}
	return Result;
}

bool UPlayerMinion::IsDefeated() const
{
	return CurrentHealth <= 0.f;
}

TArray<FCombatAbilityStruct> UPlayerMinion::GetBattleAbilities() const
{
	TArray<FCombatAbilityStruct> Abilities = { CombatMath::MakeBasicAttack() };
	for (const FCombatAbilityStruct& Ability : ClassData.Abilities)
	{
		if (Level >= Ability.RequiredLevel)
		{
			Abilities.Add(Ability);
		}
	}
	return Abilities;
}

void UPlayerMinion::RestoreToFull()
{
	CurrentHealth = GetMaxHealth();
	CurrentOvershield = static_cast<float>(CombatMath::RollRange(Defense.Overshield));
	BroadcastHealthChanged();
}

void UPlayerMinion::BroadcastHealthChanged()
{
	OnHealthChangedEvent.Broadcast(CurrentHealth, GetMaxHealth(), CurrentOvershield);
}

void UPlayerMinion::RecalculateStats()
{
	const float PreviousMaxHealth = GetMaxHealth();

	const int32 LevelsGained = Level - 1;
	BaseAttributes.Health = ClassData.BaseAttributes.Health + ClassData.AttributesPerLevel.Health * LevelsGained;
	BaseAttributes.Strength = ClassData.BaseAttributes.Strength + ClassData.AttributesPerLevel.Strength * LevelsGained;
	BaseAttributes.Intelligence = ClassData.BaseAttributes.Intelligence + ClassData.AttributesPerLevel.Intelligence * LevelsGained;
	BaseAttributes.Dexterity = ClassData.BaseAttributes.Dexterity + ClassData.AttributesPerLevel.Dexterity * LevelsGained;

	if (ModifierPool.Num() == 0)
	{
		DataTableHelpers::LoadAllRows(ItemModifierDataTablePath, ModifierPool);
	}

	auto GatherRolled = [this](const TMap<FString, int32>& Modifiers, TArray<FRolledModifier>& OutRolled)
	{
		for (const TPair<FString, int32>& Entry : Modifiers)
		{
			const FItemModifierStruct* Row = ModifierPool.FindByPredicate(
				[&Entry](const FItemModifierStruct& Modifier)
				{
					return Modifier.ModifierId.ToString().Equals(Entry.Key, ESearchCase::IgnoreCase);
				});
			if (Row)
			{
				OutRolled.Add({ Row, Entry.Value });
			}
		}
	};
	auto GatherItem = [&GatherRolled](const auto& ItemStats, TArray<FRolledModifier>& OutRolled)
	{
		GatherRolled(ItemStats.ImplicitModifiers, OutRolled);
		GatherRolled(ItemStats.PrefixModifiers, OutRolled);
		GatherRolled(ItemStats.SuffixModifiers, OutRolled);
	};

	// Weapon modifiers are kept separate for crit, which is local to the weapon.
	TArray<FRolledModifier> WeaponModifiers;
	if (HasWeaponEquipped())
	{
		GatherItem(EquippedWeapon, WeaponModifiers);
	}

	TArray<FRolledModifier> AllModifiers = WeaponModifiers;
	for (const TPair<EArmorSlot, FItemArmorStatsStruct>& Entry : EquippedArmor)
	{
		GatherItem(Entry.Value, AllModifiers);
	}

	auto ApplyAttributeModifiers = [&AllModifiers](int32 BaseValue, float FItemModifierAttributeStruct::* Field)
	{
		return FMath::RoundToInt(ApplyModifiersToStat(static_cast<float>(BaseValue), AllModifiers,
			[Field](const FItemModifierAffectedAttributes& Attributes) { return Attributes.AttributeModifier.*Field; }));
	};
	TotalAttributes.Health = ApplyAttributeModifiers(BaseAttributes.Health, &FItemModifierAttributeStruct::Health);
	TotalAttributes.Strength = ApplyAttributeModifiers(BaseAttributes.Strength, &FItemModifierAttributeStruct::Strength);
	TotalAttributes.Intelligence = ApplyAttributeModifiers(BaseAttributes.Intelligence, &FItemModifierAttributeStruct::Intelligence);
	TotalAttributes.Dexterity = ApplyAttributeModifiers(BaseAttributes.Dexterity, &FItemModifierAttributeStruct::Dexterity);

	// A saved armor item's BaseDefense already includes its local mitigation modifiers
	// (ItemHandler::RecalculateArmorDefense), so it's summed as-is.
	Defense = FCombatDefense();
	for (const TPair<EArmorSlot, FItemArmorStatsStruct>& Entry : EquippedArmor)
	{
		Defense.PhysicalMitigation += Entry.Value.BaseDefense.BasePhysicalMitigation;
		Defense.EvadeChance += Entry.Value.BaseDefense.BaseEvade;
		Defense.Overshield += Entry.Value.BaseDefense.BaseOvershield;
	}

	struct FResistanceChannel
	{
		EDamageType DamageType;
		float FItemModifierReistanceStruct::* Field;
	};
	static const FResistanceChannel ResistanceChannels[] = {
		{ EDamageType::Fire,     &FItemModifierReistanceStruct::FireResistance },
		{ EDamageType::Ice,      &FItemModifierReistanceStruct::IceResistance },
		{ EDamageType::Electric, &FItemModifierReistanceStruct::ElectricResistance },
		{ EDamageType::Abyssal,  &FItemModifierReistanceStruct::AbyssalResistance },
	};
	for (const FResistanceChannel& Channel : ResistanceChannels)
	{
		const float Resistance = ApplyModifiersToStat(0.f, AllModifiers,
			[&Channel](const FItemModifierAffectedAttributes& Attributes) { return Attributes.ResistanceModifier.*Channel.Field; });
		if (Resistance != 0.f)
		{
			Defense.Resistances.Add(Channel.DamageType, Resistance);
		}
	}

	Offense = FCombatOffense();
	Offense.Level = Level;
	Offense.Attributes = TotalAttributes;
	if (HasWeaponEquipped())
	{
		// A saved weapon's local damage and attack rate already include its damage and attack-rate
		// modifiers (ItemHandler::RecalculateWeaponLocalDamage); crit is the only local stat left to apply.
		FWeaponLocalDamage LocalDamage{};
		if (const FWeaponLocalDamage* FoundLocalDamage = EquippedWeapon.WeaponLocalDamage.Find(TEXT("LocalDamage")))
		{
			LocalDamage = *FoundLocalDamage;
		}

		Offense.WeaponDamage.Add(EDamageType::Physical, LocalDamage.LocalPhysicalDamage);
		Offense.WeaponDamage.Add(EDamageType::Fire, LocalDamage.LocalFireDamage);
		Offense.WeaponDamage.Add(EDamageType::Ice, LocalDamage.LocalIceDamage);
		Offense.WeaponDamage.Add(EDamageType::Electric, LocalDamage.LocalElectricDamage);
		Offense.WeaponDamage.Add(EDamageType::Abyssal, LocalDamage.LocalAbyssalDamage);
		Offense.AttackRate = EquippedWeapon.AttackRate;

		Offense.CritChance = ApplyModifiersToStat(EquippedWeaponBase.WeaponBaseCritChance, WeaponModifiers,
			[](const FItemModifierAffectedAttributes& Attributes) { return Attributes.WeaponCriticalChance; });
		Offense.CritMultiplier = ApplyModifiersToStat(EquippedWeaponBase.WeaponBaseCritMulti, WeaponModifiers,
			[](const FItemModifierAffectedAttributes& Attributes) { return Attributes.WeaponCriticalMulti; });
	}
	else
	{
		Offense.WeaponDamage = ClassData.Damage;
		Offense.AttackRate = ClassData.AttackRate;
	}

	// Keep the same amount of missing health across stat changes, so a full-health minion stays full after
	// equipping +health gear, but never revive a defeated minion or let an unequip kill a living one.
	if (CurrentHealth > 0.f)
	{
		const float MissingHealth = PreviousMaxHealth - CurrentHealth;
		CurrentHealth = FMath::Clamp(GetMaxHealth() - MissingHealth, 1.f, GetMaxHealth());
	}
	CurrentOvershield = FMath::Min(CurrentOvershield, static_cast<float>(FMath::Max(Defense.Overshield.X, Defense.Overshield.Y)));

	BroadcastHealthChanged();
}
