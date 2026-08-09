// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "BaseItemStruct.h"

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
    ValidateButton(SelectSwordButton);
    ValidateButton(SelectShieldButton);
    ValidateButton(ItemInfoButton);

	BuyButton->OnClicked.AddDynamic(this, &UCoreMenu::OnBuyButtonClicked);
	SellButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSellButtonClicked);
	SelectSwordButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSelectSwordButtonClicked);
	SelectShieldButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSelectShieldButtonClicked);
	ItemInfoButton->OnClicked.AddDynamic(this, &UCoreMenu::OnItemInfoButtonClicked);

	PlayerSwordCount = 1;
	PlayerMoneyCount = 20;
	LogOutputText = FText::GetEmpty();
	if (LogOutput)
	{
		LogOutput->SetText(LogOutputText);
	}

	PrintLogToScreen(FText::FromString(TEXT("CoreMenu initialized.")));
	UpdateSwordCount(PlayerSwordCount);
	UpdatePlayerMoney(PlayerMoneyCount);
}

void UCoreMenu::OnSellButtonClicked()
{
	// Empty handler for SellButton click.
    PrintLogToScreen(FText::FromString(TEXT("SellButton Clicked.")));
	UE_LOG(LogTemp, Warning, TEXT("SellButton Clicked."));
	OnSellButtonClickedEvent.Broadcast(ItemType);
}

void UCoreMenu::OnBuyButtonClicked()
{
    PrintLogToScreen(FText::FromString(TEXT("BuyButton Clicked. Event Dispatched")));
    UE_LOG(LogTemp, Warning, TEXT("BuyButton Clicked. Event Dispatched"));
    OnBuyButtonClickedEvent.Broadcast(ItemType);
}

void UCoreMenu::OnSelectSwordButtonClicked()
{
	SelectItemData(FText::FromString(TEXT("sword_1h_001")));
}

void UCoreMenu::OnSelectShieldButtonClicked()
{
	SelectItemData(FText::FromString(TEXT("shield_1h_001")));
}

void UCoreMenu::SelectItemData(const FText& ItemIdText)
{
	UDataTable* ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ItemData/BaseItem_DT"));
	if (!ItemDataTable)
	{
		PrintLogToScreen(FText::FromString(TEXT("Failed to load BaseItem_DT data table.")));
		UE_LOG(LogTemp, Error, TEXT("Failed to load BaseItem_DT data table."));
		bHasSelectedItemData = false;
		return;
	}
	
	const FString SearchText = ItemIdText.ToString();
	TArray<FName> RowNames = ItemDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("CoreMenu::SelectItemData"));
		if (!ItemRow)
		{
			continue;
		}

		if (ItemRow->ItemId.ToString().Equals(SearchText, ESearchCase::IgnoreCase))
		{
			SelectedItemData = *ItemRow;
			bHasSelectedItemData = true;

			FText ItemNameText = ItemRow->ItemName;
			PrintLogToScreen(FText::Format(FText::FromString(TEXT("Selected item name: {0}")), ItemNameText));
			UE_LOG(LogTemp, Warning, TEXT("Selected item name: %s"), *ItemNameText.ToString());
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Item Name: %s"), *ItemNameText.ToString()));
			}
			return;
		}
	}

	bHasSelectedItemData = false;
	PrintLogToScreen(FText::Format(FText::FromString(TEXT("No item found for ItemId: {0}")), FText::FromString(SearchText)));
	UE_LOG(LogTemp, Warning, TEXT("No item found for ItemId: %s"), *SearchText);
}

void UCoreMenu::OnItemInfoButtonClicked()
{
	if (!bHasSelectedItemData)
	{
		PrintLogToScreen(FText::FromString(TEXT("No item data has been selected yet.")));
		UE_LOG(LogTemp, Warning, TEXT("No item data has been selected yet."));
		return;
	}

	FString ItemClassName = UEnum::GetValueAsString(SelectedItemData.ItemClass);
	FString ItemSlotName = UEnum::GetValueAsString(SelectedItemData.ItemSlot);
	FString ItemInfo = FString::Printf(
		TEXT("Item ID: %s\nName: %s\nClass: %s\nSlot: %s"),
		*SelectedItemData.ItemId.ToString(),
		*SelectedItemData.ItemName.ToString(),
		*ItemClassName,
		*ItemSlotName);

	PrintLogToScreen(FText::Format(FText::FromString(TEXT("Item info: {0}")), FText::FromString(ItemInfo)));
	UE_LOG(LogTemp, Warning, TEXT("Item info: %s"), *ItemInfo);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, ItemInfo);
	}
}

void UCoreMenu::ValidateButton(UButton* InputButton)
{
	if (!InputButton)
	{
		PrintLogToScreen(FText::FromString(TEXT("InputButton is null or not found!")));
		UE_LOG(LogTemp, Error, TEXT("InputButton is null or not found!"));
		return;
	}

	PrintLogToScreen(FText::Format(FText::FromString(TEXT("Found button: {0}")), FText::FromString(InputButton->GetName())));
	UE_LOG(LogTemp, Warning, TEXT("Found button: %s"), *InputButton->GetName());
}

void UCoreMenu::PrintLogToScreen(const FText& Message)
{
	if (!LogOutput)
	{
		UE_LOG(LogTemp, Warning, TEXT("LogOutput widget not bound."));
		return;
	}

	FString CombinedText;
	if (!LogOutputText.IsEmpty())
	{
		CombinedText = LogOutputText.ToString() + TEXT("\n") + Message.ToString();
	}
	else
	{
		CombinedText = Message.ToString();
	}

	LogOutputText = FText::FromString(CombinedText);
	LogOutput->SetText(LogOutputText);
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


