// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "TimerManager.h"
#include "BaseItemStruct.h"
#include "CoreMenu.generated.h"

class UButton;
class FString;
class UImage;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UUniformGridPanel;
class UDataTable;
class UPanelWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuyButtonClickedEvent, FString, ItemId, FString, ItemUUID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSellButtonClickedEvent, FString, ItemId, FString, ItemUUID);
// CurrencyId is whatever SetSelectedCurrencyId last set (empty for a plain full reroll, or a
// Currency_DT row id to modify the item's existing modifiers via that currency's rules instead).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRandomizeItemEvent, FString, CurrencyId);
// Fired when a saved item is loaded from the PlayerStashUniGrid, so listeners (e.g. ItemHandler) can
// prep that existing item - identified by ItemId and its already-assigned ItemUUID - as the active item.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStashItemSelectedEvent, FString, ItemId, FString, ItemUUID);
// Fired when an item is clicked in the ShopUniGrid, so listeners (e.g. ItemHandler) can cache its
// base stats and display them as the active item, the same way a stash selection or randomize does.
// ItemUUID is the UUID minted for this shop listing back in PopulateShopGrid.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopItemSelectedEvent, FString, ItemId, FString, ItemUUID);
// Fired when the Reset Game button is clicked, so listeners (e.g. ItemHandler, PlayerInventory)
// can wipe their own persisted save data (SavedItems.json, SavedCurrency.json) and reset to defaults.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResetGameEvent);

class UCoreMenu;

// Carries a saved item's UUID for a dynamically created load-item button, since
// UButton::OnClicked takes no parameters and can't otherwise identify its sender.
UCLASS()
class ULoadItemButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString ItemUUID;

	UPROPERTY()
	TObjectPtr<UCoreMenu> OwningMenu;

	UFUNCTION()
	void HandleClicked();
};

// Carries an item's ItemId and its pre-minted ItemUUID (assigned in PopulateShopGrid) for a
// dynamically created shop grid item button, since UButton::OnClicked takes no parameters and
// can't otherwise identify its sender.
UCLASS()
class UShopItemButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString ItemId;

	UPROPERTY()
	FString ItemUUID;

	UPROPERTY()
	TObjectPtr<UCoreMenu> OwningMenu;

	UFUNCTION()
	void HandleClicked();
};

/**
 *
 */
UCLASS()
class CHAOSRECIPE_API UCoreMenu : public UUserWidget
{
	GENERATED_BODY()

	friend class ULoadItemButtonProxy;
	friend class UShopItemButtonProxy;

public:


	// Event that can be bound from Blueprint when the Buy button is clicked.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBuyButtonClickedEvent OnBuyButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSellButtonClickedEvent OnSellButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRandomizeItemEvent OnRandomizeItemEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStashItemSelectedEvent OnStashItemSelectedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnShopItemSelectedEvent OnShopItemSelectedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnResetGameEvent OnResetGameButtonClickedEvent;

	UPROPERTY()
	int32 PlayerSwordCount;

	UPROPERTY()
	FText LogOutputText;

	UFUNCTION(BlueprintCallable, Category = "Player Inventory Display")
	void UpdateSwordCount(int32 PlayerSwords);

	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void LogToScreen(const FString& NewMessage);

	// Sets the text shown in the Load Item box's ActiveItemTextBox.
	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void SetActiveItemText(const FString& NewMessage);

	// Sets the text shown in GoldHorizBox's PlayerGoldTextBox to the given gold count.
	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void SetPlayerGoldText(int32 NewGoldCount);

	// Sets the text shown in WarningsHorizBox's WarningsTextBox.
	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void SetWarningText(const FString& NewMessage);

	// UUID of the saved item currently loaded via the Load Item box (empty if none).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString GetSelectedItemUUID() const { return SelectedItemUUID; }

	// Clears the currently loaded UUID, e.g. once it has been sold.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSelectedItemUUID() { SelectedItemUUID.Empty(); }

	// Sets which currency (a Currency_DT row id, or empty for a plain full reroll) the next Randomize
	// click will use. Meant to be called from a future currency-selection widget (e.g. a combo box bound
	// to CurrencyDataTableRowNames below) - no such widget exists yet, so this currently defaults to
	// empty (full reroll) until one calls it.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSelectedCurrencyId(const FString& CurrencyId) { SelectedCurrencyId = CurrencyId; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString GetSelectedCurrencyId() const { return SelectedCurrencyId; }

	// Rebuilds ShopUniGrid from CurrentShopItems without changing which items are listed
	// (unlike RandomizeShopItems, which rolls a new set first). Useful for refreshing the
	// shop's buttons/icons after external state changes (e.g. gold updates).
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RefreshShopGrid();

	// Removes an item from CurrentShopItems (by ItemId) and refreshes the grid. Called by ItemHandler
	// once a purchase has actually succeeded, so a rejected (unaffordable) buy leaves the shop unchanged.
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RemoveItemFromShop(const FString& ItemId);

	UPROPERTY()
	TArray<FName> ItemDataTableRowNames;

	UPROPERTY()
	TArray<FName> CurrentShopItems;

protected:
	virtual void NativeConstruct() override;

	// UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	// UTextBlock* TestTextBlock;

	// Bound from the widget blueprint (named 'SellButton')
	UPROPERTY(meta = (BindWidget))
	UButton* SellButton;
	// Bound from the widget blueprint (named 'BuyButton')
	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;
	UPROPERTY(meta = (BindWidget))
	UButton* RandomizeButton;
	UPROPERTY(meta = (BindWidget))
	UButton* ShopButton;
	UPROPERTY(meta = (BindWidget))
	UButton* RandomizeShopButton;
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* ShopWindowBox;
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* ShopUniGrid;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* ShopWindowHeader;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActiveItemTextBox;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* ActiveItemImageHorizBox;
	UPROPERTY(meta = (BindWidget))
	UImage* ActiveItemImage;
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* PlayerStashUniGrid;
	UPROPERTY(meta = (BindWidget))
	UButton* PlayerStashButton;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* PlayerStashHorizBox;
	UPROPERTY(meta = (BindWidget))
	UButton* StashSelectButton;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerGoldTextBox;
	UPROPERTY(meta = (BindWidget))
	UButton* ResetGameButton;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* WarningsHorizBox;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WarningsTextBox;

	// Click handler for SellButton
	UFUNCTION()
	void OnSellButtonClicked();
	// Click handler for SellButton
	UFUNCTION()
	void OnBuyButtonClicked();
	// Generic item lookup helper
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SelectItemData(const FText& ItemIdText);
	// Click handler for the randomize item button
	UFUNCTION()
	void OnRandomizeItemButtonClicked();
	// Click handler for a dynamically created saved-item button
	UFUNCTION()
	void OnSingleLoadItemButtonClicked(FString ItemUUID);
	// Click handler for the Shop button; populates ShopUniGrid with every item in BaseItem_DT
	UFUNCTION()
	void OnShopButtonClicked();
	// Click handler for the Randomize Shop button
	UFUNCTION()
	void OnRandomizeShopButtonClicked();
	// Click handler for a dynamically created shop grid item button
	UFUNCTION()
	void OnShopItemButtonClicked(FString ItemId, FString ItemUUID);
	// Click handler for the Player Stash button; shows PlayerStashHorizBox
	UFUNCTION()
	void OnPlayerStashButtonClicked();
	// Click handler for the Stash Select button; hides both the shop and stash panels
	UFUNCTION()
	void OnStashSelectButtonClicked();
	// Click handler for the Reset Game button; wipes save data (broadcasts OnResetGameButtonClickedEvent
	// for listeners like ItemHandler/PlayerInventory) and refreshes the stash and shop grids.
	UFUNCTION()
	void OnResetGameButtonClicked();
	// Clears and repopulates ShopUniGrid with an item button + icon for every row in BaseItem_DT
	void PopulateShopGrid();
	// Clears and repopulates PlayerStashUniGrid with a WBP_SingleImageButton for every saved item (via ItemInstanceManager)
	void PopulatePlayerStash();
	// Sets the visibility of a panel widget and all of its children, recursively
	void SetPanelAndChildrenVisibility(UPanelWidget* Panel, ESlateVisibility NewVisibility);
	// Sets the visibility of each given panel (and its children)
	void UpdatePanelVisibility(const TArray<UPanelWidget*>& Panels, ESlateVisibility NewVisibility);
	// Shows ActiveItemImageHorizBox (and its children) and sets ActiveItemImage to the selected item's icon
	void ShowActiveItemImage();

	UFUNCTION()
	void ValidateButton(UButton* InputButton);

	UFUNCTION()
	void RandomizeShopItems();

	UPROPERTY()
	int32 ShopItemCount = 8;

	UPROPERTY()
	int32 ItemDataTableRowCount;

	UPROPERTY()
	int32 Cost = 5;

	UPROPERTY()
	FString ItemType = TEXT("Sword");

	// Shared reference to BaseItem_DT, loaded once in NativeConstruct.
	UPROPERTY()
	UDataTable* ItemDataTable;

	// Shared reference to Currency_DT, loaded once in NativeConstruct.
	UPROPERTY()
	UDataTable* CurrencyDataTable;

	// Every row name in Currency_DT, so Blueprint can populate a currency-selection widget without
	// needing its own data table reference.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FName> CurrencyDataTableRowNames;

	// CurrencyId to use for the next Randomize click (empty = full reroll). Set via SetSelectedCurrencyId.
	UPROPERTY()
	FString SelectedCurrencyId;

	UPROPERTY()
	FBaseItemStruct SelectedItemData;

	UPROPERTY()
	FString SelectedItemId; // 

	// UUID of the saved item currently loaded via the Load Item box, cleared once sold.
	UPROPERTY()
	FString SelectedItemUUID;

	UPROPERTY()
	bool bHasSelectedItemData = false;

	// Keeps the per-button proxies alive (and their click bindings valid) between shop grid repopulations.
	UPROPERTY()
	TArray<TObjectPtr<UShopItemButtonProxy>> ShopItemButtonProxies;

	// Keeps the per-button proxies alive (and their click bindings valid) between player stash repopulations.
	UPROPERTY()
	TArray<TObjectPtr<ULoadItemButtonProxy>> PlayerStashButtonProxies;

	// Clears WarningsTextBox; bound to WarningTextTimerHandle by SetWarningText, never called directly
	// so that clearing the text never re-arms the timer.
	UFUNCTION()
	void ClearWarningText();

	FTimerHandle WarningTextTimerHandle;

};
