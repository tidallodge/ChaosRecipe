// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StoreManager.generated.h"

class UCoreMenu;
class FString;

// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OParam(FStoreSale, int32, ItemValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStoreSale, FString, ItemType, int32, ItemValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStoreBuy, FString, ItemType, int32, ItemValue);

/**
 * 
 * 
 */
UCLASS()
class CHAOSRECIPE_API UStoreManager : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void BindToCoreMenuEvents(UCoreMenu* CoreMenu);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FStoreSale OnStoreSale;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FStoreBuy OnStoreBuy;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "StoreData")
	int32 ItemValue = 5;

	UFUNCTION()
	void OnStoreSaleEvent(FString ItemType);
	UFUNCTION()
	void OnStoreBuyEvent(FString ItemType);

/** NEEDED FUNCTIONS
	Startup function called from another widget to connect delegates and prompt any other logic needed on startup (maybe DB connection)

	Listen to CoreMenu Sell Button
	Pass Selected Item and Item's Value to Player Inventory on Sell Button Click
		- This will need to know which items the player has selected somehow
			- Can probably pass an array from CoreMenu to This with item selected Item Types

	Source full item list from database

	Select which items are displayed for player to buy
	Pass displayed items to display widget
 */

};
