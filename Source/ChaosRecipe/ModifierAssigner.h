// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemModifierStruct.h"

/**
 * Receives the full pool of possible item modifiers (loaded from ItemModifier_DT)
 * and is responsible for rolling/assigning modifiers onto an item.
 */
class CHAOSRECIPE_API ModifierAssigner
{
public:
	ModifierAssigner();
	explicit ModifierAssigner(const TArray<FItemModifierStruct>& InModifierPool);
	~ModifierAssigner();

	// Replaces the current pool of every possible item modifier.
	void SetModifierPool(const TArray<FItemModifierStruct>& InModifierPool);

	// Assigns each ModifierId a contiguous block of numbers sized to its ModifierWeight,
	// accumulates the total weight, and logs each ModifierId with its range.
	void BuildWeightRanges();

	const TArray<FItemModifierStruct>& GetModifierPool() const { return ModifierPool; }
	int32 GetModifierPoolCount() const { return ModifierPool.Num(); }

	const TMap<FString, FInt32Range>& GetModifierWeightRanges() const { return ModifierWeightRanges; }
	int32 GetTotalModifierWeight() const { return TotalModifierWeight; }

private:
	// Every modifier row pulled from ItemModifier_DT, kept for later rolling/assignment.
	TArray<FItemModifierStruct> ModifierPool;

	// ModifierId -> [start, end] weighted number range used for weighted random selection.
	TMap<FString, FInt32Range> ModifierWeightRanges;

	// Sum of every modifier's weight; upper bound (exclusive) for a weighted roll.
	int32 TotalModifierWeight = 0;
};
