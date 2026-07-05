// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "CoreMenu.generated.h"

class UButton;
class FString;
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

	UFUNCTION(BlueprintCallable, Category = "Player Inventory Display")
	void UpdateSwordCount(int32 PlayerSwords);

	UFUNCTION(BlueprintCallable, Category = "Player Inventory Display")
	void UpdatePlayerMoney(int32 PlayerMoney);

protected:
	virtual void NativeConstruct() override;
	
	// Bound from the widget blueprint (named 'SellButton')
	UPROPERTY(meta = (BindWidget))
	UButton* SellButton;
	// Bound from the widget blueprint (named 'SellButton')
	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;

	// Click handler for SellButton
	UFUNCTION()
	void OnSellButtonClicked();
	// Click handler for SellButton
	UFUNCTION()
	void OnBuyButtonClicked();

	UFUNCTION()
	void ValidateButton(UButton* InputButton);

	UPROPERTY()
	int32 Cost = 5;

	UPROPERTY()
	FString ItemType = TEXT("Sword");


};
