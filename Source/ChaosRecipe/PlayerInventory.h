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
	// rerolled, so the roll's gold cost can be charged to PlayerGoldCount, and (if CurrencyId is set)
	// one unit of that currency's stack can be consumed.
	UFUNCTION()
	void HandleItemRandomized(int32 GoldCost, FString CurrencyId);

	// Single entry point for changing PlayerGoldCount (positive to add, negative to charge), so
	// every source of a gold change (sell, buy, randomize, future shop actions, ...) persists to
	// SavedCurrency.json and refreshes the display the same way instead of duplicating that logic.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AdjustPlayerGoldCount(int32 GoldDelta);

	// Bound to CoreMenu's OnResetGameButtonClickedEvent: deletes SavedCurrency.json and resets
	// PlayerGoldCount back to DefaultPlayerGoldCount.
	UFUNCTION()
	void OnResetGame();

	// Queried by ItemHandler before granting a purchased item, so an unaffordable buy is rejected
	// before the item is saved to the stash rather than after (which would grant it for free).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanAffordGoldCost(int32 GoldCost) const;

	// Queried by ItemHandler before spending a currency on a randomize, so an unaffordable currency
	// choice is rejected before any modifiers change.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanAffordCurrency(const FString& CurrencyId, int32 Count = 1) const;

	// Single entry point for changing a currency's stack count (positive to add, negative to spend), so
	// every source of a currency change persists to SavedCurrency.json the same way AdjustPlayerGoldCount
	// does for gold. Clamped at 0 - there is no other system yet that grants currencies to the player.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AdjustCurrencyStackCount(const FString& CurrencyId, int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetCurrencyStackCount(const FString& CurrencyId) const;

protected:
	// Pushes PlayerGoldCount to the bound CoreMenu's PlayerGoldTextBox, if a CoreMenu has been bound.
	void UpdatePlayerGoldDisplay() const;

	static constexpr int32 DefaultPlayerGoldCount = 250;

	int32 PlayerGoldCount = DefaultPlayerGoldCount;

	// CurrencyId -> how many the player holds. Nothing grants currencies to the player yet (no shop/drop
	// system for them), so this starts empty unless a previous save already has stacks in it.
	TMap<FString, int32> CurrencyStackCounts;

	TUniquePtr<UCurrencyManager> CurrencyManager;

	UPROPERTY()
	TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;
};
