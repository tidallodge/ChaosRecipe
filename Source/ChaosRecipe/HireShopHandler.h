// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinionClassStruct.h"
#include "HireShopHandler.generated.h"

class UCoreMenu;
class UPlayerInventory;

// Everything SavedHireShop.json keeps: the minions the hire shop offers.
USTRUCT()
struct FHireShopSaveData
{
	GENERATED_BODY()

	// The class of each minion on offer, in the order CoreMenu lists them.
	UPROPERTY()
	TArray<EMinionClass> MinionClasses;
};

UCLASS()
class CHAOSRECIPE_API UHireShopHandler : public UObject
{
	GENERATED_BODY()

public:
	// Loads the minions on offer from SavedHireShop.json (rolling a first set if nothing's saved yet) and shows them
	// in CoreMenu's hire shop, then rerolls them whenever its HireShopRerollButton is clicked.
	UFUNCTION()
	void BindToCoreMenuEvents(UCoreMenu* CoreMenu);

	// Lets a reroll check for, and charge, RerollGoldCost.
	UFUNCTION()
	void BindToPlayerInventory(UPlayerInventory* PlayerInventory);

	// Bound to CoreMenu's OnHireShopRerollButtonClickedEvent: replaces every minion on offer with a new random set of
	// MaxHireShopMinions, for RerollGoldCost. Rejected if the player can't afford it.
	UFUNCTION()
	void OnRerollHireShop();

	// Bound to CoreMenu's OnResetGameButtonClickedEvent: rolls a fresh set for free, replacing the saved one.
	UFUNCTION()
	void OnResetGame();

	// How many minions the hire shop offers at once.
	UPROPERTY(EditAnywhere, Category = "Hire Shop", meta = (ClampMin = "0"))
	int32 MaxHireShopMinions = 3;

	// Gold charged for each reroll.
	UPROPERTY(EditAnywhere, Category = "Economy")
	int32 RerollGoldCost = 100;

protected:
	// Replaces HireShopMinions with MaxHireShopMinions random MinionClass_DT classes (repeats allowed). Returns false,
	// leaving them as they were, if MinionClass_DT has no rows.
	bool RollHireShopMinions();

	// Loads HireShopMinions from SavedHireShop.json. Returns false if there's no save (or it can't be read).
	bool LoadHireShopMinions();
	void SaveHireShopMinions() const;

	// Pushes HireShopMinions to the bound CoreMenu's hire shop, if a CoreMenu has been bound.
	void UpdateHireShopDisplay() const;

	// The class of each minion the hire shop offers.
	UPROPERTY()
	TArray<EMinionClass> HireShopMinions;

	UPROPERTY()
	TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;

	UPROPERTY()
	TObjectPtr<UPlayerInventory> BoundPlayerInventory = nullptr;
};
