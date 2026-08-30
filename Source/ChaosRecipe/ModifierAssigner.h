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

	int32 ModifierCount = 3;

	// Replaces the current pool of every possible item modifier.
	void SetModifierPool(const TArray<FItemModifierStruct>& InModifierPool);

	// Builds the subset of modifiers valid for the given item class/type, then rolls
	// `Count` unique ModifierIds via weighted selection and returns them.
	TArray<FString> AssignModifiers(const FString& ItemId, EItemClass ItemClass, const FString& ItemType, int32 InModifierCount);

	const TArray<FItemModifierStruct>& GetModifierPool() const { return ModifierPool; }
	int32 GetModifierPoolCount() const { return ModifierPool.Num(); }

	const TMap<FString, FInt32Range>& GetModifierWeightRanges() const { return ModifierWeightRanges; }
	int32 GetTotalModifierWeight() const { return TotalModifierWeight; }

private:
	// True if the modifier is valid for the given item class or specific item type.
	bool ModifierMatchesItem(const FItemModifierStruct& Modifier, EItemClass ItemClass, const FString& ItemType) const;

	// Shuffles the candidates, assigns each a weight-sized number block, rolls one number
	// across the total weight, and returns the ModifierId that owns it (logs each step).
	FString RollWeightedModifier(TArray<FItemModifierStruct>& Candidates);

	// Every modifier row pulled from ItemModifier_DT, kept for later rolling/assignment.
	TArray<FItemModifierStruct> ModifierPool;

	// ModifierId -> [start, end] weighted number range used for weighted random selection.
	TMap<FString, FInt32Range> ModifierWeightRanges;

	// Sum of every modifier's weight; upper bound (exclusive) for a weighted roll.
	int32 TotalModifierWeight = 0;
};
