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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuyButtonClickedEvent, FString, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSellButtonClickedEvent, FString, ItemType);

/**
 * 
 */
UCLASS()
class CHAOSRECIPE_API UCoreMenu : public UUserWidget
{
	GENERATED_BODY()

public:


	// Event that can be bound from Blueprint when the Buy button is clicked.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBuyButtonClickedEvent OnBuyButtonClickedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSellButtonClickedEvent OnSellButtonClickedEvent;

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
	UButton* ItemInfoButton;

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
	// Generic item lookup helper
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SelectItemData(const FText& ItemIdText);
	// Click handler for the item info button
	UFUNCTION()
	void OnItemInfoButtonClicked();

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

};
