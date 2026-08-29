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

	for (const FItemModifierStruct& Modifier : ModifierPool)
	{
		UE_LOG(LogTemp, Verbose, TEXT("ModifierAssigner:  - %s (%s)"),
			*Modifier.ModifierId.ToString(),
			*UEnum::GetValueAsString(Modifier.ModifierAffixType));
	}
}
