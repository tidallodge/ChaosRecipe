// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UCoreMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get OwningPlayer PlayerController!"));
	}

    ValidateButton(BuyButton);
    ValidateButton(SellButton);

	BuyButton->OnClicked.AddDynamic(this, &UCoreMenu::OnBuyButtonClicked);
	SellButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSellButtonClicked);

	PlayerSwordCount = 1;
	PlayerMoneyCount = 20;

	UpdateSwordCount(PlayerSwordCount);
	UpdatePlayerMoney(PlayerMoneyCount);
}

void UCoreMenu::OnSellButtonClicked()
{
	// Empty handler for SellButton click.
    UE_LOG(LogTemp, Warning, TEXT("SellButton Clicked."));
	OnSellButtonClickedEvent.Broadcast(ItemType);
}

void UCoreMenu::OnBuyButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("BuyButton Clicked. Event Dispatched"));
    OnBuyButtonClickedEvent.Broadcast(ItemType);
}

void UCoreMenu::ValidateButton(UButton* InputButton)
{
	if (!InputButton)
	{
		UE_LOG(LogTemp, Error, TEXT("InputButton is null or not found!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Found button: %s"), *InputButton->GetName());
}

void UCoreMenu::UpdateSwordCount(int32 PlayerSwords)
{
	if (SwordCountText)
	{
		FText SwordText = FText::Format(FText::FromString("Swords: {0}"), FText::AsNumber(PlayerSwords));
		SwordCountText->SetText(SwordText);
	}
}

void UCoreMenu::UpdatePlayerMoney(int32 PlayerMoney)
{
	if (PlayerMoneyTextBlock)
	{
		FText PlayerMoneyText = FText::Format(FText::FromString("Player Bank: {0}"), FText::AsNumber(PlayerMoney));
		PlayerMoneyTextBlock->SetText(PlayerMoneyText);
	}
}


