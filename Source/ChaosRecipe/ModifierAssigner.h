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

	const TArray<FItemModifierStruct>& GetModifierPool() const { return ModifierPool; }
	int32 GetModifierPoolCount() const { return ModifierPool.Num(); }

private:
	// Every modifier row pulled from ItemModifier_DT, kept for later rolling/assignment.
	TArray<FItemModifierStruct> ModifierPool;
};
