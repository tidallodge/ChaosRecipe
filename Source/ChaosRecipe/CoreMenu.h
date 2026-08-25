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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuyButtonClickedEvent, FString, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSellButtonClickedEvent, FString, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemInfoButtonClickedEvent, FString, ItemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRandomizeItemEvent, float, RandomValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveItemButtonClickedEvent, FString, ItemId);

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

/**
 *
 */
UCLASS()
class CHAOSRECIPE_API UCoreMenu : public UUserWidget
{
	GENERATED_BODY()

	friend class ULoadItemButtonProxy;

public:


	// Event that can be bound from Blueprint when the Buy button is clicked.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBuyButtonClickedEvent OnBuyButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSellButtonClickedEvent OnSellButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemInfoButtonClickedEvent OnItemInfoButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRandomizeItemEvent OnRandomizeItemEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSaveItemButtonClickedEvent OnSaveItemButtonClickedEvent;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SwordCountText;
	UPROPERTY()
	int32 PlayerSwordCount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerMoneyTextBlock;
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

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TestTextBlock;

	UPROPERTY(meta = (BindWidget))
	UImage* Item1_Icon;
	UPROPERTY(meta = (BindWidget))
	UImage* Item2_Icon;
	UPROPERTY(meta = (BindWidget))
	UImage* Item3_Icon;
	UPROPERTY(meta = (BindWidget))
	UImage* Item4_Icon;

	// Bound from the widget blueprint (named 'SellButton')
	UPROPERTY(meta = (BindWidget))
	UButton* SellButton;
	// Bound from the widget blueprint (named 'BuyButton')
	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;
	UPROPERTY(meta = (BindWidget))
	UButton* Item1;
	UPROPERTY(meta = (BindWidget))
	UButton* Item2;
	UPROPERTY(meta = (BindWidget))
	UButton* Item3;
	UPROPERTY(meta = (BindWidget))
	UButton* Item4;
	UPROPERTY(meta = (BindWidget))
	UButton* ItemInfoButton;
	UPROPERTY(meta = (BindWidget))
	UButton* RandomizeButton;
	UPROPERTY(meta = (BindWidget))
	UButton* SaveItemButton;
	UPROPERTY(meta = (BindWidget))
	UButton* LoadItemButton;
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* LoadItemVertBox;
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* LoadItemHeaderBox;
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* LoadItemHorizBox;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActiveItemTextBox;

	// Click handler for SellButton
	UFUNCTION()
	void OnSellButtonClicked();
	// Click handler for SellButton
	UFUNCTION()
	void OnBuyButtonClicked();
	// Click handler for the sword selection button
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnSelectSwordButtonClicked();
	// Click handler for the axe selection button
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnSelectAxeButtonClicked();
	// Click handler for the shield selection button
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnSelectShieldButtonClicked();
	// Click handler for the 1H axe selection button
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnSelectHatchetButtonClicked();
	// Generic item lookup helper
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SelectItemData(const FText& ItemIdText);
	// Click handler for the item info button
	UFUNCTION()
	void OnItemInfoButtonClicked();
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
	// Click handler for the close ("X") button that hides LoadItemHorizBox
	UFUNCTION()
	void OnCloseLoadItemBoxButtonClicked();
	// Creates the "X" close button in LoadItemHeaderBox
	void CreateCloseLoadItemButton();

	UFUNCTION()
	void ValidateButton(UButton* InputButton);

	UPROPERTY()
	int32 Cost = 5;

	UPROPERTY()
	FString ItemType = TEXT("Sword");

	UPROPERTY()
	FBaseItemStruct SelectedItemData;

	UPROPERTY()
	FString SelectedItemId;

	UPROPERTY()
	bool bHasSelectedItemData = false;

	// Guards against creating the "X" close button more than once across repeated LoadItem clicks.
	UPROPERTY()
	bool bCloseLoadItemButtonCreated = false;

	// Keeps the per-button proxies alive (and their click bindings valid) between repopulations.
	UPROPERTY()
	TArray<TObjectPtr<ULoadItemButtonProxy>> LoadItemButtonProxies;

};
