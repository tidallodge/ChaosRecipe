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
	BuildWeightRanges();
}

void ModifierAssigner::BuildWeightRanges()
{
	ModifierWeightRanges.Reset();
	TotalModifierWeight = 0;

	// Randomize the order the modifiers are assigned ranges in (Fisher-Yates shuffle).
	for (int32 i = ModifierPool.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		ModifierPool.Swap(i, j);
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: assigning weight ranges for %d modifiers:"), ModifierPool.Num());

	for (const FItemModifierStruct& Modifier : ModifierPool)
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

	if (TotalModifierWeight <= 0)
	{
		return;
	}

	// Weighted roll: pick a number in [0, TotalModifierWeight - 1] and resolve the modifier that owns it.
	const int32 RolledNumber = FMath::RandRange(0, TotalModifierWeight - 1);
	FString RolledModifierId = TEXT("None");
	for (const TPair<FString, FInt32Range>& RangePair : ModifierWeightRanges)
	{
		if (RangePair.Value.Contains(RolledNumber))
		{
			RolledModifierId = RangePair.Key;
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ModifierAssigner: RolledNumber=%d -> ModifierId=%s"), RolledNumber, *RolledModifierId);
}
