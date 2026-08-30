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
		// TODO - this looks like it requires class and type match? probably clean this up to handle the way the DT is structured, match on class unless there's a type override
		if (ModifierMatchesItem(Modifier, ItemClass, ItemType))
		{
			MatchingModifiers.Add(Modifier);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ModifierAssigner: AssignModifiers for ItemId=%s Class=%s Type=%s -> %d matching modifiers."),
		*ItemId, *UEnum::GetValueAsString(ItemClass), *ItemType, MatchingModifiers.Num());

	// An item may carry at most this many prefix and this many suffix modifiers.
	const int32 MaxPrefixes = 3;
	const int32 MaxSuffixes = 3;
	int32 PrefixCount = 0;
	int32 SuffixCount = 0;

	// sub to add mods up to mod count limit
	while (AssignedModifierIds.Num() < InModifierCount && MatchingModifiers.Num() > 0)
	{
		const FString RolledId = RollWeightedModifier(MatchingModifiers);
		if (RolledId.IsEmpty())
		{
			break;
		}

		AssignedModifierIds.AddUnique(RolledId);

		// Record which affix slot the rolled modifier consumed.
		EAffixType RolledAffix = EAffixType::Implicit;
		if (const FItemModifierStruct* RolledRow = MatchingModifiers.FindByPredicate(
			[&RolledId](const FItemModifierStruct& Modifier)
			{
				return Modifier.ModifierId.ToString().Equals(RolledId, ESearchCase::IgnoreCase);
			}))
		{
			RolledAffix = RolledRow->ModifierAffixType;
		}

		if (RolledAffix == EAffixType::Prefix)
		{
			++PrefixCount;
		}
		else if (RolledAffix == EAffixType::Suffix)
		{
			++SuffixCount;
		}

		// Reduce the candidate list to modifiers that still validate against everything assigned
		// so far: this drops the just-rolled modifier and anything sharing one of its buckets.
		const TArray<FString> ValidModifierIds = ValidateModifierPool(MatchingModifiers, AssignedModifierIds);
		MatchingModifiers.RemoveAll([&ValidModifierIds](const FItemModifierStruct& Modifier)
		{
			const FString ModifierId = Modifier.ModifierId.ToString();
			return !ValidModifierIds.ContainsByPredicate([&ModifierId](const FString& ValidId)
			{
				return ModifierId.Equals(ValidId, ESearchCase::IgnoreCase);
			});
		});

		// Once an affix slot is full, drop every remaining modifier of that affix type so
		// subsequent rolls can only draw from the still-available slot(s).
		const bool bPrefixFull = PrefixCount >= MaxPrefixes;
		const bool bSuffixFull = SuffixCount >= MaxSuffixes;
		if (bPrefixFull || bSuffixFull)
		{
			MatchingModifiers.RemoveAll([bPrefixFull, bSuffixFull](const FItemModifierStruct& Modifier)
			{
				return (bPrefixFull && Modifier.ModifierAffixType == EAffixType::Prefix)
					|| (bSuffixFull && Modifier.ModifierAffixType == EAffixType::Suffix);
			});

			UE_LOG(LogTemp, Warning,
				TEXT("ModifierAssigner: affix cap hit (prefix=%d/%d, suffix=%d/%d); %d candidates remain."),
				PrefixCount, MaxPrefixes, SuffixCount, MaxSuffixes, MatchingModifiers.Num());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: assigned %d modifiers to ItemId=%s:"), AssignedModifierIds.Num(), *ItemId);
	for (const FString& AssignedId : AssignedModifierIds)
	{
		UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner:  -> %s"), *AssignedId);
	}

	return AssignedModifierIds;
}

const FModifierBuckets* ModifierAssigner::FindModifierBuckets(const TArray<FItemModifierStruct>& InModifierPool, const FString& ModifierId)
{
	const FItemModifierStruct* Row = InModifierPool.FindByPredicate(
		[&ModifierId](const FItemModifierStruct& Modifier)
		{
			return Modifier.ModifierId.ToString().Equals(ModifierId, ESearchCase::IgnoreCase);
		});

	return Row != nullptr ? &Row->ModifierBuckets : nullptr;
}

TArray<FModifierBuckets> ModifierAssigner::GetAssignedModifierBuckets(const TArray<FItemModifierStruct>& InModifierPool, const TArray<FString>& InAssignedModifierIds)
{
	TArray<FModifierBuckets> AssignedBuckets;

	for (const FString& AssignedId : InAssignedModifierIds)
	{
		if (const FModifierBuckets* Buckets = FindModifierBuckets(InModifierPool, AssignedId))
		{
			AssignedBuckets.Add(*Buckets);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: assigned ModifierId=%s not found in pool; skipping."), *AssignedId);
		}
	}

	return AssignedBuckets;
}

bool ModifierAssigner::BucketsOverlap(const FModifierBuckets& A, const FModifierBuckets& B)
{
	// Walk every bool UPROPERTY on FModifierBuckets so new buckets are picked up automatically.
	for (TFieldIterator<FBoolProperty> It(FModifierBuckets::StaticStruct()); It; ++It)
	{
		const FBoolProperty* Prop = *It;
		if (Prop->GetPropertyValue_InContainer(&A) && Prop->GetPropertyValue_InContainer(&B))
		{
			return true;
		}
	}

	return false;
}

bool ModifierAssigner::IsModifierInAssignedBuckets(const FModifierBuckets& ModifierBuckets, const TArray<FModifierBuckets>& AssignedBuckets)
{
	for (const FModifierBuckets& Assigned : AssignedBuckets)
	{
		if (BucketsOverlap(ModifierBuckets, Assigned))
		{
			return true;
		}
	}

	return false;
}

TArray<FString> ModifierAssigner::ValidateModifierPool(const TArray<FItemModifierStruct>& InModifierPool, const TArray<FString>& InAssignedModifierIds)
{
	// Build the list of buckets currently occupied by the assigned modifiers.
	const TArray<FModifierBuckets> AssignedBuckets = GetAssignedModifierBuckets(InModifierPool, InAssignedModifierIds);

	UE_LOG(LogTemp, Warning,
		TEXT("ModifierAssigner: ValidateModifierPool - %d assigned modifiers, %d bucket sets resolved, %d pool modifiers."),
		InAssignedModifierIds.Num(), AssignedBuckets.Num(), InModifierPool.Num());

	// Collect every pool modifier that is not already assigned and whose buckets do not
	// collide with an occupied bucket - these are still valid to roll for this item.
	TArray<FString> ValidModifierIds;
	for (const FItemModifierStruct& Modifier : InModifierPool)
	{
		const FString ModifierId = Modifier.ModifierId.ToString();

		const bool bAlreadyAssigned = InAssignedModifierIds.ContainsByPredicate(
			[&ModifierId](const FString& AssignedId)
			{
				return ModifierId.Equals(AssignedId, ESearchCase::IgnoreCase);
			});

		if (bAlreadyAssigned || IsModifierInAssignedBuckets(Modifier.ModifierBuckets, AssignedBuckets))
		{
			continue;
		}

		ValidModifierIds.Add(ModifierId);
		UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner:  valid -> %s"), *ModifierId);
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: %d / %d pool modifiers still valid."), ValidModifierIds.Num(), InModifierPool.Num());
	return ValidModifierIds;
}
