// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "TimerManager.h"
#include "BaseItemStruct.h"
#include "CombatTypes.h"
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
class UProgressBar;
class UPlayerMinion;
class UEnemy;
class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuyButtonClickedEvent, FString, ItemId, FString, ItemUUID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSellButtonClickedEvent, FString, ItemId, FString, ItemUUID);
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
// Fired when BattlePlayButton in BattleWindow is clicked, so listeners (e.g. BattleManager) can start a battle.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattlePlayButtonClickedEvent);
// Fired when StashAssignButton is clicked, so listeners (e.g. MinionHandler) can equip the stash item ItemUUID on
// the minion MinionUUID. Expected to be handled synchronously: CoreMenu reloads the stash right after broadcasting.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssignItemToMinionEvent, FString, MinionUUID, FString, ItemUUID);
// Bound by ItemHandler to supply the text a shop/stash item's hover flyout shows (the same text ActiveItemTextBox
// shows for it), since CoreMenu doesn't build item stats itself.
DECLARE_DELEGATE_RetVal_TwoParams(FString, FGetItemDisplayTextDelegate, const FString& /* ItemId */, const FString& /* ItemUUID */);

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

	// Bound to the button's ToolTipWidgetDelegate, so the item's hover flyout is built each time it opens.
	UFUNCTION()
	UWidget* CreateToolTip();
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

	// Bound to the button's ToolTipWidgetDelegate, so the item's hover flyout is built each time it opens.
	UFUNCTION()
	UWidget* CreateToolTip();
};

// Carries a currency's Currency_DT RowName for a dynamically created currency grid button, since
// UButton::OnClicked takes no parameters and can't otherwise identify its sender.
UCLASS()
class UCurrencyButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName RowName;

	UPROPERTY()
	TObjectPtr<UCoreMenu> OwningMenu;

	UFUNCTION()
	void HandleClicked();
};

// Carries the minion a dynamically created WBP_MinionButton stands for, since UButton::OnClicked takes no
// parameters and can't otherwise identify its sender.
UCLASS()
class UMinionButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UPlayerMinion> Minion;

	UPROPERTY()
	TObjectPtr<UCoreMenu> OwningMenu;

	UFUNCTION()
	void HandleClicked();
};

// One damage number shown in a FDamageNumberStack.
USTRUCT()
struct FDamageNumber
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UTextBlock> Text;

	// Seconds since it was shown. It fades out as this approaches UCoreMenu::DamageNumberLifetime.
	float Age = 0.f;
};

// The damage numbers shown where an HP text (BattleMinionHPText/BattleEnemyHPText) used to be, oldest on top.
USTRUCT()
struct FDamageNumberStack
{
	GENERATED_BODY()

	// Put in the HP text's place by UCoreMenu::InitDamageNumberStack; null if that failed.
	UPROPERTY()
	TObjectPtr<UVerticalBox> Box;

	// The HP text itself, no longer shown, kept as the template every number copies its look from.
	UPROPERTY()
	TObjectPtr<UTextBlock> Template;

	// In the same order as Box's children: oldest first.
	UPROPERTY()
	TArray<FDamageNumber> Numbers;
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
	friend class UCurrencyButtonProxy;
	friend class UMinionButtonProxy;

public:


	// Event that can be bound from Blueprint when the Buy button is clicked.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBuyButtonClickedEvent OnBuyButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSellButtonClickedEvent OnSellButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStashItemSelectedEvent OnStashItemSelectedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnShopItemSelectedEvent OnShopItemSelectedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnResetGameEvent OnResetGameButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBattlePlayButtonClickedEvent OnBattlePlayButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAssignItemToMinionEvent OnAssignItemToMinionEvent;

	// Supplies the text for each shop/stash item's hover flyout (CreateItemToolTip). Unbound, items show no flyout.
	FGetItemDisplayTextDelegate GetItemDisplayTextDelegate;

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

	// Caches the player's currency stack counts (CurrencyId -> count) and updates the count text shown
	// over each icon in InscriptionUniGrid/GlyphUniGrid. Called by PlayerInventory whenever a stack changes.
	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void SetCurrencyStackCounts(const TMap<FString, int32>& StackCounts);

	// Sets the text shown in WarningsHorizBox's WarningsTextBox. It stays fully visible for DisplaySeconds, then
	// fades out over WarningTextFadeSeconds; a DisplaySeconds of 0 or less keeps it up until the next warning.
	UFUNCTION(BlueprintCallable, Category = "Menu Text")
	void SetWarningText(const FString& NewMessage, float DisplaySeconds = 3.f);

	// UUID of the saved item currently loaded via the Load Item box (empty if none).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString GetSelectedItemUUID() const { return SelectedItemUUID; }

	// Clears the currently loaded UUID, e.g. once it has been sold.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSelectedItemUUID() { SetSelectedItemUUID(FString()); }

	// Sets which currency (a Currency_DT row id) is selected for rolling the active item. Called when a currency
	// button in InscriptionUniGrid/GlyphUniGrid is clicked; empty until one is.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSelectedCurrencyId(const FString& CurrencyId) { SelectedCurrencyId = CurrencyId; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString GetSelectedCurrencyId() const { return SelectedCurrencyId; }

	// The button inside ActiveItemRollButton, which ItemHandler binds to roll the active item with the selected
	// currency. Null if WBP_SingleImageButton has no SingleImageButton.
	UButton* GetActiveItemRollButton() const;

	// Rebuilds ShopUniGrid from CurrentShopItems without changing which items are listed
	// (unlike RandomizeShopItems, which rolls a new set first). Useful for refreshing the
	// shop's buttons/icons after external state changes (e.g. gold updates).
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RefreshShopGrid();

	// Removes an item from CurrentShopItems (by ItemId, preferring the selected listing) and refreshes the
	// grid. Called by ItemHandler once a purchase has actually succeeded, so a rejected (unaffordable) buy
	// leaves the shop unchanged.
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RemoveItemFromShop(const FString& ItemId);

	// Shows Minion's health in BattleMinionHPBar and keeps it updated as it changes (hits, equipment, level,
	// restores), and shows the damage of every hit it takes as a number where BattleMinionHPText was, until
	// another minion is selected. Pass null to clear the display.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetSelectedMinion(UPlayerMinion* Minion);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	UPlayerMinion* GetSelectedMinion() const { return SelectedMinion; }

	// Same as SetSelectedMinion, for BattleEnemyHPBar/BattleEnemyHPText.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetSelectedEnemy(UEnemy* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	UEnemy* GetSelectedEnemy() const { return SelectedEnemy; }

	// Rebuilds MinionButtonsVertBox with a WBP_MinionButton for each of Minions, up to MaxMinionButtons, showing
	// its class's MinionIcon and ClassName. Called by MinionHandler whenever a minion is created or loaded.
	UFUNCTION(BlueprintCallable, Category = "Minion")
	void SetMinionButtons(const TArray<UPlayerMinion*>& Minions);

	// MinionUUID of the minion last clicked in MinionButtonsVertBox (empty if none has been yet).
	UFUNCTION(BlueprintCallable, Category = "Minion")
	FString GetSelectedMinionUUID() const { return SelectedMinionUUID; }

	// Shows BattleTime (seconds since the battle started) as "M:SS.s" in BattleTimerText, if the widget
	// blueprint has one. Called by BattleManager every battle tick.
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetBattleTimerText(float BattleTime);

	UPROPERTY()
	TArray<FName> ItemDataTableRowNames;

	UPROPERTY()
	TArray<FName> CurrentShopItems;

	// Each CurrentShopItems listing's UUID (same index), minted when the shop is rolled so a listing keeps
	// its UUID, and its selection, across grid rebuilds.
	UPROPERTY()
	TArray<FString> CurrentShopItemUUIDs;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	// UTextBlock* TestTextBlock;

	// Bound from the widget blueprint (named 'SellButton')
	UPROPERTY(meta = (BindWidget))
	UButton* SellButton;
	// Bound from the widget blueprint (named 'BuyButton')
	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;
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
	// Only shown while ActiveItemImageHorizBox is (see SetActiveItemTextVisibility)
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* ActiveItemTextHorizBox;
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* ActiveItemTextVertBox;
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
	// Each passes the item selected in PlayerStashUniGrid on to CraftStashItem/AssignStashItem
	UPROPERTY(meta = (BindWidget))
	UButton* StashCraftButton;
	UPROPERTY(meta = (BindWidget))
	UButton* StashAssignButton;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerGoldTextBox;
	UPROPERTY(meta = (BindWidget))
	UButton* ResetGameButton;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* WarningsHorizBox;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WarningsTextBox;
	// Nested in CurrencyVertBox; filled by PopulateCurrencyGrids
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* InscriptionUniGrid;
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* GlyphUniGrid;
	// WBP_SingleImageButton showing the currently selected currency's icon and flyout; clicking it rolls the active
	// item with that currency (bound by ItemHandler, via GetActiveItemRollButton)
	UPROPERTY(meta = (BindWidget))
	UUserWidget* ActiveItemRollButton;
	// Nested in BattleMinionVertBox/BattleEnemyVertBox; driven by the selected minion/enemy (SetSelectedMinion/SetSelectedEnemy).
	// The HP texts are swapped out for damage number stacks at runtime (see InitDamageNumberStack) and only
	// kept as the look those numbers copy.
	UPROPERTY(meta = (BindWidget))
	UProgressBar* BattleMinionHPBar;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* BattleMinionHPText;
	UPROPERTY(meta = (BindWidget))
	UProgressBar* BattleEnemyHPBar;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* BattleEnemyHPText;
	// Nested in BattleWindow
	UPROPERTY(meta = (BindWidget))
	UButton* BattlePlayButton;
	// Optional: add a Text Block with this name to WBP_CoreMenu to show the running battle time.
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BattleTimerText;
	// Filled with a WBP_MinionButton per minion by SetMinionButtons
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* MinionButtonsVertBox;
	// Shown by OnMinionButtonClicked, with MinionEquipText listing the clicked minion's equipped items
	// (UpdateMinionEquipText). Named to match WBP_CoreMenu, typo included.
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* MinionEqiupVertBox;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* MinionEquipText;

	// Click handler for SellButton
	UFUNCTION()
	void OnSellButtonClicked();
	// Click handler for SellButton
	UFUNCTION()
	void OnBuyButtonClicked();
	// Generic item lookup helper
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SelectItemData(const FText& ItemIdText);
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
	// Click handler for the Player Stash button; toggles PlayerStashHorizBox (hiding the active item display when
	// it opens, but leaving the shop as it is)
	UFUNCTION()
	void OnPlayerStashButtonClicked();
	// Click handler for the Stash Craft button; hides the shop panel, shows the selected stash item in the active
	// item display (which hides the stash) and passes its UUID on to CraftStashItem
	UFUNCTION()
	void OnStashCraftButtonClicked();
	// Click handler for the Stash Assign button; shows the minion window (ShowMinionEquipWindow) and passes the
	// selected stash item's UUID on to AssignStashItem, leaving the stash open
	UFUNCTION()
	void OnStashAssignButtonClicked();
	// Returns whether the selection is a saved stash item, warning the player if it isn't (e.g. nothing is
	// selected, or the selection is a shop listing).
	bool ValidateSelectedStashItem();
	// Not implemented yet. Called by OnStashCraftButtonClicked with the selected stash item's UUID.
	void CraftStashItem(const FString& ItemUUID);
	// Assigns the stash item ItemUUID to the minion selected in MinionButtonsVertBox (via OnAssignItemToMinionEvent),
	// replacing whatever that minion had in the item's slot, then reloads the stash without it. Says in
	// WarningsTextBox which item was added and which, if any, were removed.
	void AssignStashItem(const FString& ItemUUID);
	// Unloads the selected item (its data, UUID and the active item text), e.g. once it's been sold or assigned.
	void ClearSelectedItem();
	// The BaseItem_DT ItemName of the saved item ItemUUID, or ItemUUID itself if it can't be found.
	FString GetSavedItemName(const FString& ItemUUID) const;
	// Click handler for the Reset Game button; wipes save data (broadcasts OnResetGameButtonClickedEvent
	// for listeners like ItemHandler/PlayerInventory) and refreshes the stash and shop grids.
	UFUNCTION()
	void OnResetGameButtonClicked();
	// Click handler for BattlePlayButton; broadcasts OnBattlePlayButtonClickedEvent
	UFUNCTION()
	void OnBattlePlayButtonClicked();
	// Clears and repopulates ShopUniGrid with an item button + icon for every row in BaseItem_DT
	void PopulateShopGrid();
	// Clears and repopulates PlayerStashUniGrid with a WBP_SingleImageButton for every saved item (via ItemInstanceManager)
	// that isn't assigned to a minion (via MinionInstanceManager). Assigned items stay in SavedItems.json.
	void PopulatePlayerStash();
	// Clears and repopulates InscriptionUniGrid and GlyphUniGrid with a WBP_SingleImageButton for every
	// Currency_DT row whose CurrencyName contains "Inscription" or "Glyph" respectively
	void PopulateCurrencyGrids();
	// Click handler for a dynamically created currency grid button; selects that currency and shows
	// its icon on ActiveItemRollButton
	UFUNCTION()
	void OnCurrencyButtonClicked(FName RowName);
	// Click handler for a dynamically created WBP_MinionButton; selects that minion's UUID and says so in
	// WarningsTextBox, then shows its equipment in the minion window (ShowMinionEquipWindow)
	UFUNCTION()
	void OnMinionButtonClicked(UPlayerMinion* Minion);
	// Shows MinionEqiupVertBox, hiding the shop and active item display. Leaves the stash as it is.
	void ShowMinionEquipWindow();
	// Sets MinionEquipText to EquipTextMinion's class name, followed by a line for each ItemSlot it has an item
	// in, naming that item ("No minion selected" if no minion button has been clicked yet).
	void UpdateMinionEquipText();
	// Bound to EquipTextMinion's OnSaveDataChangedEvent, so MinionEquipText keeps up as its equipment changes
	// (e.g. an assigned item).
	UFUNCTION()
	void OnEquipTextMinionSaveDataChanged(FString MinionUUID);
	// Sets IconImage's brush to the CurrencyIcon of the Currency_DT row named RowName
	void LoadCurrencyIcon(UImage* IconImage, const FName& RowName);
	// Builds the hover flyout for a currency button: CurrencyName on top, CurrencyDescription underneath.
	// Returns null if the Currency_DT row can't be found.
	UWidget* CreateCurrencyToolTip(const FName& RowName);
	// Builds the hover flyout for a shop/stash item: its name on top, with the rest of the text ActiveItemTextBox
	// shows for it (from GetItemDisplayTextDelegate) underneath. Returns null if there's no text for it.
	UWidget* CreateItemToolTip(const FString& ItemId, const FString& ItemUUID);
	// The look shared by every hover flyout: Title in bold over Body, on a dark background.
	UWidget* CreateTitledToolTip(const FText& Title, const FText& Body);
	// Sets the count text over CurrencyId's icon in InscriptionUniGrid/GlyphUniGrid from CurrencyStackCounts
	void UpdateCurrencyCountText(const FString& CurrencyId);
	// Sets the visibility of a panel widget and all of its children, recursively
	void SetPanelAndChildrenVisibility(UPanelWidget* Panel, ESlateVisibility NewVisibility);
	// Sets the visibility of each given panel (and its children)
	void UpdatePanelVisibility(const TArray<UPanelWidget*>& Panels, ESlateVisibility NewVisibility);
	// Shows ActiveItemImageHorizBox (and its children) along with the active item's text, sets ActiveItemImage
	// to the selected item's icon, and hides PlayerStashHorizBox and MinionEqiupVertBox
	void ShowActiveItemImage();
	// Hides ActiveItemImageHorizBox (and its children) along with the active item's text
	void HideActiveItemImage();
	// Gives ActiveItemTextHorizBox and ActiveItemTextVertBox NewVisibility, so the active item's text is only
	// up while ActiveItemImageHorizBox is
	void SetActiveItemTextVisibility(ESlateVisibility NewVisibility);
	// Shows the active item display when neither ShopWindowBox nor PlayerStashHorizBox is visible and an
	// item is selected; hides it otherwise. Call after changing either panel's visibility.
	void RefreshActiveItemDisplay();
	// Sets SelectedItemUUID and moves the white selection border onto that item's icon in the shop/stash grids.
	void SetSelectedItemUUID(const FString& ItemUUID);
	// Wraps a shop/stash grid item widget in a border (added to Borders under ItemUUID) that shows white
	// while ItemUUID is the selected item. Returns the border, to add to the grid in the item's place.
	UBorder* WrapInSelectionBorder(UWidget* ItemWidget, const FString& ItemUUID, TMap<FString, TObjectPtr<UBorder>>& Borders);
	// Shows the selection border on whichever grid item matches SelectedItemUUID and hides it on the rest.
	void RefreshSelectionBorders();

	UFUNCTION()
	void ValidateButton(UButton* InputButton);

	UFUNCTION()
	void RandomizeShopItems();

	UPROPERTY()
	int32 ShopItemCount = 8;

	// Column count for InscriptionUniGrid and GlyphUniGrid; rows grow as needed.
	UPROPERTY()
	int32 CurrencyGridColumns = 3;

	// Width/height (px) of each currency icon slot in InscriptionUniGrid and GlyphUniGrid.
	UPROPERTY()
	float CurrencyIconSize = 64.f;

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

	// The selected currency's CurrencyId (empty until one is selected). Set via SetSelectedCurrencyId.
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

	// ItemUUID -> the selection border around that item's icon, rebuilt with the shop/stash grids.
	UPROPERTY()
	TMap<FString, TObjectPtr<UBorder>> ShopItemSelectionBorders;

	UPROPERTY()
	TMap<FString, TObjectPtr<UBorder>> StashItemSelectionBorders;

	// Width (px) of the white border drawn around the selected item's icon.
	UPROPERTY()
	float ItemSelectionBorderWidth = 3.f;

	// Keeps the per-button proxies alive (and their click bindings valid) between currency grid repopulations.
	UPROPERTY()
	TArray<TObjectPtr<UCurrencyButtonProxy>> CurrencyButtonProxies;

	// Last stack counts pushed by PlayerInventory (CurrencyId -> count), kept so PopulateCurrencyGrids can
	// show them on rebuild.
	UPROPERTY()
	TMap<FString, int32> CurrencyStackCounts;

	// CurrencyId -> the count text over that currency's icon in InscriptionUniGrid/GlyphUniGrid.
	UPROPERTY()
	TMap<FString, TObjectPtr<UTextBlock>> CurrencyCountTextBlocks;

	// Font size of the count text over each currency icon.
	UPROPERTY()
	int32 CurrencyCountFontSize = 12;

	// Font sizes and wrap width (px) of the currency and item hover flyouts built by CreateTitledToolTip.
	UPROPERTY()
	int32 ToolTipTitleFontSize = 14;

	UPROPERTY()
	int32 ToolTipBodyFontSize = 11;

	UPROPERTY()
	float ToolTipWrapWidth = 280.f;

	// The item flyout last built by CreateItemToolTip. Only Slate holds on to a flyout built for
	// ToolTipWidgetDelegate, so this keeps its widgets from being garbage collected while it's up.
	UPROPERTY()
	TObjectPtr<UWidget> ItemToolTip;

	// Most minion buttons SetMinionButtons shows; minions past this many get no button.
	UPROPERTY()
	int32 MaxMinionButtons = 8;

	// Keeps the per-button proxies alive (and their click bindings valid) between minion button repopulations.
	UPROPERTY()
	TArray<TObjectPtr<UMinionButtonProxy>> MinionButtonProxies;

	// MinionUUID of the minion last clicked in MinionButtonsVertBox. Set via OnMinionButtonClicked.
	UPROPERTY()
	FString SelectedMinionUUID;

	// The minion last clicked in MinionButtonsVertBox, whose equipped items MinionEquipText lists.
	UPROPERTY()
	TObjectPtr<UPlayerMinion> EquipTextMinion;

	// Ages the current warning by DeltaTime, fading WarningsTextBox out once its DisplaySeconds are up and
	// clearing it once the fade is done.
	void TickWarningText(float DeltaTime);

	// Seconds WarningsTextBox takes to fade out once a warning's DisplaySeconds are up.
	UPROPERTY(EditAnywhere, Category = "Menu Text")
	float WarningTextFadeSeconds = 1.f;

	// Seconds the current warning has been shown, and how long it stays fully visible (0 when nothing is
	// counting down).
	float WarningTextAge = 0.f;
	float WarningTextDisplaySeconds = 0.f;

	// Bound to the selected minion's/enemy's OnHealthChangedEvent by SetSelectedMinion/SetSelectedEnemy.
	UFUNCTION()
	void OnSelectedMinionHealthChanged(float CurrentHealth, float MaxHealth, float CurrentOvershield);
	UFUNCTION()
	void OnSelectedEnemyHealthChanged(float CurrentHealth, float MaxHealth, float CurrentOvershield);

	// Bound to the selected minion's/enemy's OnHitTakenEvent by SetSelectedMinion/SetSelectedEnemy.
	UFUNCTION()
	void OnSelectedMinionHitTaken(const FCombatHitResult& HitResult);
	UFUNCTION()
	void OnSelectedEnemyHitTaken(const FCombatHitResult& HitResult);

	// Fills HPBar to Current / Max, with any overshield added to both sides. While there's overshield the
	// bar uses ShieldedHPBarColor, otherwise UnshieldedBarColor. A MaxHealth of 0 (e.g. a minion/enemy that
	// hasn't been initialized) clears it instead.
	void SetHealthDisplay(UProgressBar* HPBar, const FLinearColor& UnshieldedBarColor,
		float CurrentHealth, float MaxHealth, float CurrentOvershield);

	// Empties HPBar and puts it back to UnshieldedBarColor, for when nothing is selected.
	void ClearHealthDisplay(UProgressBar* HPBar, const FLinearColor& UnshieldedBarColor);

	// Swaps HPText out of the layout for an empty vertical box in the same spot (same slot settings), which
	// becomes Stack's Box, and keeps HPText as Stack's Template. Must run before the Slate widgets are built.
	void InitDamageNumberStack(UTextBlock* HPText, FDamageNumberStack& Stack);

	// Adds the damage HitResult did as a new number at the bottom of Stack. Hits that did no damage
	// (evaded or fully mitigated) don't show anything.
	void ShowDamageNumber(FDamageNumberStack& Stack, const FCombatHitResult& HitResult);

	// Ages every number in Stack by DeltaTime, fading it out, and removes the ones past DamageNumberLifetime.
	void TickDamageNumbers(FDamageNumberStack& Stack, float DeltaTime);

	void ClearDamageNumbers(FDamageNumberStack& Stack);

	UPROPERTY()
	FDamageNumberStack MinionDamageNumbers;

	UPROPERTY()
	FDamageNumberStack EnemyDamageNumbers;

	// Seconds each damage number takes to fade from fully visible to gone. Every number fades on its own
	// clock, so several hits close together show as a stack that empties from the top.
	UPROPERTY(EditAnywhere, Category = "Combat")
	float DamageNumberLifetime = 1.f;

	// Fill color for an HP bar while its minion/enemy has overshield left.
	UPROPERTY(EditAnywhere, Category = "Combat")
	FLinearColor ShieldedHPBarColor = FLinearColor::Blue;

	// Each HP bar's own fill color (from WBP_CoreMenu, or the override colors below), captured in
	// NativeConstruct so it can be restored once a shield is gone.
	UPROPERTY()
	FLinearColor MinionHPBarDefaultColor;

	UPROPERTY()
	FLinearColor EnemyHPBarDefaultColor;

	// Off by default so the HP bar fill colors set in WBP_CoreMenu are kept; turn on in the widget's
	// Class Defaults to have NativeConstruct apply the colors below instead.
	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bOverrideHPBarColors = false;

	UPROPERTY(EditAnywhere, Category = "Combat", meta = (EditCondition = "bOverrideHPBarColors"))
	FLinearColor MinionHPBarColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, Category = "Combat", meta = (EditCondition = "bOverrideHPBarColors"))
	FLinearColor EnemyHPBarColor = FLinearColor::Red;

	UPROPERTY()
	TObjectPtr<UPlayerMinion> SelectedMinion;

	UPROPERTY()
	TObjectPtr<UEnemy> SelectedEnemy;

};
