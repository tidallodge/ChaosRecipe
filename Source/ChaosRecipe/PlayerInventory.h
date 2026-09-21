// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerInventory.generated.h"

class UCoreMenu;
class UStoreManager;
class UItemHandler;
class UCurrencyManager;

UCLASS()
class CHAOSRECIPE_API UPlayerInventory : public UObject
{
	GENERATED_BODY()

public:
	UPlayerInventory();
	// Required alongside the destructor below whenever a UCLASS holds a TUniquePtr to a
	// forward-declared type (UCurrencyManager here): UHT's generated vtable-helper constructor
	// would otherwise need to destroy that member against an incomplete type. Both this and the
	// destructor are defined in the .cpp, where CurrencyManager.h is fully included.
	UPlayerInventory(FVTableHelper& Helper);
	virtual ~UPlayerInventory() override;

	UFUNCTION()
	void BindToCoreMenuEvents(UCoreMenu* CoreMenu);
	UFUNCTION()
	void BindToStoreManagerEvents(UStoreManager* StoreManager);
	UFUNCTION()
	void BindToItemHandlerEvents(UItemHandler* ItemHandler);

	UFUNCTION()
	void HandleStoreBuy(FString ItemType, FString ItemUUID);

	// Bound to ItemHandler's OnItemSoldEvent: that single shared ItemHandler instance (see
	// CoreGameMode::BeginPlay) resolves the sold item's gold value and broadcasts it here before it
	// removes the item from SavedItems.json, so PlayerGoldCount is always updated first.
	UFUNCTION()
	void HandleItemSold(FString ItemId, FString ItemUUID, float GoldValue);

	// Bound to ItemHandler's OnItemBoughtEvent: fired once the purchased item's (post-modifier)
	// gold value has been resolved, so it can be charged to PlayerGoldCount.
	UFUNCTION()
	void HandleItemBought(FString ItemId, FString ItemUUID, float GoldValue);

	// Bound to ItemHandler's OnItemRandomizedEvent: fired each time an item is successfully
	// rerolled, so the roll's gold cost can be charged to PlayerGoldCount.
	UFUNCTION()
	void HandleItemRandomized(int32 GoldCost);

	// Single entry point for changing PlayerGoldCount (positive to add, negative to charge), so
	// every source of a gold change (sell, buy, randomize, future shop actions, ...) persists to
	// SavedCurrency.json and refreshes the display the same way instead of duplicating that logic.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AdjustPlayerGoldCount(int32 GoldDelta);

protected:
	// Pushes PlayerGoldCount to the bound CoreMenu's PlayerGoldTextBox, if a CoreMenu has been bound.
	void UpdatePlayerGoldDisplay() const;

	int32 PlayerGoldCount = 0;

	TUniquePtr<UCurrencyManager> CurrencyManager;

	UPROPERTY()
	TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;
};
