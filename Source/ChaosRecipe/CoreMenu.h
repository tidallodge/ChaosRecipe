// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/RichTextBlock.h"
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuyButtonClickedEvent, FString, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSellButtonClickedEvent, FString, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRandomizeItemEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveItemButtonClickedEvent, FString, ItemId);
// Fired when a saved item is loaded from the PlayerStashUniGrid, so listeners (e.g. ItemHandler) can
// prep that existing item - identified by ItemId and its already-assigned ItemUUID - as the active item.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStashItemSelectedEvent, FString, ItemId, FString, ItemUUID);
// Fired when an item is clicked in the ShopUniGrid, so listeners (e.g. ItemHandler) can cache its
// base stats and display them as the active item, the same way a stash selection or randomize does.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopItemSelectedEvent, FString, ItemId);

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

// Carries an item's ItemId for a dynamically created shop grid item button, since
// UButton::OnClicked takes no parameters and can't otherwise identify its sender.
UCLASS()
class UShopItemButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString ItemId;

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
	FOnSaveItemButtonClickedEvent OnSaveItemButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStashItemSelectedEvent OnStashItemSelectedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnShopItemSelectedEvent OnShopItemSelectedEvent;

	UPROPERTY()
	int32 PlayerSwordCount;

	UPROPERTY()
	int32 PlayerMoneyCount;

	// UPROPERTY(meta = (BindWidget))
	// URichTextBlock* LogOutput;
	UPROPERTY()
	FText LogOutputText;

	UFUNCTION(BlueprintCallable, Category = "Player Inventory Display")
	void UpdateSwordCount(int32 PlayerSwords);

	UFUNCTION(BlueprintCallable, Category = "Player Inventory Display")
	void UpdatePlayerMoney(int32 PlayerMoney);

	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void LogToScreen(const FString& NewMessage);

	// Sets the text shown in the Load Item box's ActiveItemTextBox.
	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void SetActiveItemText(const FString& NewMessage);

	// UUID of the saved item currently loaded via the Load Item box (empty if none).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString GetSelectedItemUUID() const { return SelectedItemUUID; }

	// Clears the currently loaded UUID, e.g. once it has been sold.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSelectedItemUUID() { SelectedItemUUID.Empty(); }

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
	UButton* SaveItemButton;
	UPROPERTY(meta = (BindWidget))
	UButton* LoadItemButton;
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
	UUniformGridPanel* PlayerStashUniGrid;
	UPROPERTY(meta = (BindWidget))
	UButton* PlayerStashButton;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* PlayerStashHorizBox;

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
	// Click handler for the save item button
	UFUNCTION()
	void OnSaveItemButtonClicked();
	// Click handler for the load item button
	UFUNCTION()
	void OnLoadItemButtonClicked();
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
	void OnShopItemButtonClicked(FString ItemId);
	// Click handler for the Player Stash button; shows PlayerStashHorizBox
	UFUNCTION()
	void OnPlayerStashButtonClicked();
	// Clears and repopulates ShopUniGrid with an item button + icon for every row in BaseItem_DT
	void PopulateShopGrid();
	// Clears and repopulates PlayerStashUniGrid with a WBP_SingleImageButton for every saved item (via ItemInstanceManager)
	void PopulatePlayerStash();
	// Sets the visibility of a panel widget and all of its direct children
	void SetPanelAndChildrenVisibility(UPanelWidget* Panel, ESlateVisibility NewVisibility);

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

};
