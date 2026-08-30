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

	// Returns the ModifierIds from InModifierPool that are still valid to roll given the
	// already-assigned modifiers: not already assigned and no overlapping bucket. Logs the result.
	TArray<FString> ValidateModifierPool(const TArray<FItemModifierStruct>& InModifierPool, const TArray<FString>& InAssignedModifierIds);

private:
	// True if the modifier is valid for the given item class or specific item type.
	bool ModifierMatchesItem(const FItemModifierStruct& Modifier, EItemClass ItemClass, const FString& ItemType) const;

	// Shuffles the candidates, assigns each a weight-sized number block, rolls one number
	// across the total weight, and returns the ModifierId that owns it (logs each step).
	FString RollWeightedModifier(TArray<FItemModifierStruct>& Candidates);

	// Looks up the ModifierBuckets struct for a single ModifierId within the given pool.
	static const FModifierBuckets* FindModifierBuckets(const TArray<FItemModifierStruct>& InModifierPool, const FString& ModifierId);

	// Collects the ModifierBuckets struct of every currently-assigned modifier.
	static TArray<FModifierBuckets> GetAssignedModifierBuckets(const TArray<FItemModifierStruct>& InModifierPool, const TArray<FString>& InAssignedModifierIds);

	// True if any bucket flag is set on both structs (reflection-driven, no hard-coded field list).
	static bool BucketsOverlap(const FModifierBuckets& A, const FModifierBuckets& B);

	// True if the modifier's buckets overlap with any of the already-assigned buckets.
	static bool IsModifierInAssignedBuckets(const FModifierBuckets& ModifierBuckets, const TArray<FModifierBuckets>& AssignedBuckets);

	// Every modifier row pulled from ItemModifier_DT, kept for later rolling/assignment.
	TArray<FItemModifierStruct> ModifierPool;

	// Set a temporary array for validating modifiers for rolling
	TArray<FString> TempModifierPoolUUIDs;

	// Set a temp array for assigned modifier buckets (collected from resolved DT rows)
	TArray<FModifierBuckets> TempModifierBuckets;

	// ModifierId -> [start, end] weighted number range used for weighted random selection.
	TMap<FString, FInt32Range> ModifierWeightRanges;

	// Sum of every modifier's weight; upper bound (exclusive) for a weighted roll.
	int32 TotalModifierWeight = 0;
};
