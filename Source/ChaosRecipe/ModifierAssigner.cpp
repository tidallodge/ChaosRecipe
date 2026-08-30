// Fill out your copyright notice in the Description page of Project Settings.


#include "ModifierAssigner.h"

ModifierAssigner::ModifierAssigner()
{
}

ModifierAssigner::ModifierAssigner(const TArray<FItemModifierStruct>& InModifierPool)
	: ModifierPool(InModifierPool)
{
	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: constructed with %d possible modifiers."), ModifierPool.Num());
}

ModifierAssigner::~ModifierAssigner()
{
}

void ModifierAssigner::SetModifierPool(const TArray<FItemModifierStruct>& InModifierPool)
{
	ModifierPool = InModifierPool;
	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: received %d possible modifiers from ItemModifier_DT."), ModifierPool.Num());
}

bool ModifierAssigner::ModifierMatchesItem(const FItemModifierStruct& Modifier, EItemClass ItemClass, const FString& ItemType) const
{
	// Match on the broad item class first.
	const FValidItemClasses& Classes = Modifier.ValidItemClasses;
	switch (ItemClass)
	{
	case EItemClass::Armor:   if (Classes.Armor)   return true; break;
	case EItemClass::Weapon:  if (Classes.Weapon)  return true; break;
	case EItemClass::Shield:  if (Classes.Shield)  return true; break;
	case EItemClass::Jewelry: if (Classes.Jewelry) return true; break;
	case EItemClass::Misc:    if (Classes.Misc)    return true; break;
	default: break;
	}

	// Then match on the specific item type override.
	const FValidItemTypesOverride& Types = Modifier.ValidItemTypesOverride;
	auto TypeIs = [&ItemType](const TCHAR* Name) { return ItemType.Equals(Name, ESearchCase::IgnoreCase); };

	if (TypeIs(TEXT("Sword")) && Types.Weapon.Sword)   return true;
	if (TypeIs(TEXT("Axe"))   && Types.Weapon.Axe)     return true;

	if (TypeIs(TEXT("Head"))   && Types.Armor.Head)    return true;
	if (TypeIs(TEXT("Chest"))  && Types.Armor.Chest)   return true;
	if (TypeIs(TEXT("Gloves")) && Types.Armor.Gloves)  return true;
	if (TypeIs(TEXT("Boots"))  && Types.Armor.Boots)   return true;
	if (TypeIs(TEXT("Shield")) && Types.Armor.Shield)  return true;

	if (TypeIs(TEXT("HandOff")) && Types.Shield.HandOff) return true;

	if (TypeIs(TEXT("Neck")) && Types.Jewelry.Neck) return true;
	if (TypeIs(TEXT("Ring")) && Types.Jewelry.Ring) return true;
	if (TypeIs(TEXT("Belt")) && Types.Jewelry.Belt) return true;

	if (TypeIs(TEXT("Misc")) && Types.Misc.Misc) return true;

	return false;
}

FString ModifierAssigner::RollWeightedModifier(TArray<FItemModifierStruct>& Candidates)
{
	ModifierWeightRanges.Reset();
	TotalModifierWeight = 0;

	// Randomize the order the modifiers are assigned ranges in (Fisher-Yates shuffle).
	for (int32 i = Candidates.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Candidates.Swap(i, j);
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: assigning weight ranges for %d candidate modifiers:"), Candidates.Num());


	// sub to compile list of valid modifier candidates to a pool to pick from
	for (const FItemModifierStruct& Modifier : Candidates)
	{
		const FString ModifierId = Modifier.ModifierId.ToString();
		const int32 Weight = FMath::Max(0, Modifier.ModifierWeight);

		if (Weight <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("ModifierId=%s | Weight=0 | no range assigned"), *ModifierId);
			continue;
		}

		// Each modifier owns a contiguous block of numbers [RangeStart, RangeEnd] sized to its weight.
		const int32 RangeStart = TotalModifierWeight;
		const int32 RangeEnd = TotalModifierWeight + Weight - 1;

		ModifierWeightRanges.Add(ModifierId, FInt32Range::Inclusive(RangeStart, RangeEnd));
		TotalModifierWeight += Weight;

		UE_LOG(LogTemp, Warning, TEXT("ModifierId=%s | Range=[%d, %d]"), *ModifierId, RangeStart, RangeEnd);
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: TotalModifierWeight=%d"), TotalModifierWeight);

	// exit if no mods
	if (TotalModifierWeight <= 0)
	{
		return FString();
	}

	// Weighted roll: pick a number in [0, TotalModifierWeight - 1] and resolve the modifier that owns it.
	const int32 RolledNumber = FMath::RandRange(0, TotalModifierWeight - 1);
	FString RolledModifierId;
	for (const TPair<FString, FInt32Range>& RangePair : ModifierWeightRanges)
	{
		if (RangePair.Value.Contains(RolledNumber))
		{
			RolledModifierId = RangePair.Key;
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: RolledNumber=%d -> ModifierId=%s"), RolledNumber, *RolledModifierId);
	return RolledModifierId;
}

TArray<FString> ModifierAssigner::AssignModifiers(const FString& ItemId, EItemClass ItemClass, const FString& ItemType, int32 InModifierCount)
{
	TArray<FString> AssignedModifierIds;

	// Build the list of modifiers valid for this item's class or specific type.
	TArray<FItemModifierStruct> MatchingModifiers;
	for (const FItemModifierStruct& Modifier : ModifierPool)
	{
		if (ModifierMatchesItem(Modifier, ItemClass, ItemType))
		{
			MatchingModifiers.Add(Modifier);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ModifierAssigner: AssignModifiers for ItemId=%s Class=%s Type=%s -> %d matching modifiers."),
		*ItemId, *UEnum::GetValueAsString(ItemClass), *ItemType, MatchingModifiers.Num());

	// sub to add mods up to mod count limit
	while (AssignedModifierIds.Num() < InModifierCount && MatchingModifiers.Num() > 0)
	{
		// TODO - add a new function to pass the newly rolled modifier below that will remove all conflicting modifiers from the MatchingModifiers TArray set at the start of this function
		const FString RolledId = RollWeightedModifier(MatchingModifiers);
		if (RolledId.IsEmpty())
		{
			break;
		}

		AssignedModifierIds.AddUnique(RolledId);

		// TODO - rework this to pass the RolledId to the function made in the above TODO to update the list of matching modifiers
		// Remove the picked modifier so the next roll produces a unique ModifierId.
		MatchingModifiers.RemoveAll([&RolledId](const FItemModifierStruct& Modifier)
		{
			return Modifier.ModifierId.ToString().Equals(RolledId, ESearchCase::IgnoreCase);
		});
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: assigned %d modifiers to ItemId=%s:"), AssignedModifierIds.Num(), *ItemId);
	for (const FString& AssignedId : AssignedModifierIds)
	{
		UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner:  -> %s"), *AssignedId);
	}

	return AssignedModifierIds;
}
