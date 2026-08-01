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
	ItemInfoButton->OnClicked.AddDynamic(this, &UCoreMenu::OnItemInfoButtonClicked);

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

void UCoreMenu::OnSelectSwordButtonClicked()
{
	UDataTable* ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ItemData/BaseItem_DT"));
	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load BaseItem_DT data table."));
		return;
	}

	FName RowName = TEXT("sword_1h_001");
	FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("CoreMenu::OnSelectSwordButtonClicked"));
	if (!ItemRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("No row found for item ID: %s"), *RowName.ToString());
		return;
	}

	SelectedItemData = *ItemRow;
	bHasSelectedItemData = true;

	FString ItemName = ItemRow->ItemName.ToString();
	UE_LOG(LogTemp, Warning, TEXT("Selected item name: %s"), *ItemName);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Item Name: %s"), *ItemName));
	}
}

void UCoreMenu::OnItemInfoButtonClicked()
{
	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No item data has been selected yet."));
		return;
	}

	FString ItemInfo = FString::Printf(
		TEXT("Item ID: %s\nName: %s\nBase Type: %s\nSlot: %s\nClass: %s\nHas Tags: %s\nMax Implicit Mods: %d\nMax Prefix Mods: %d\nMax Suffix Mods: %d"),
		*SelectedItemData.ItemId.ToString(),
		*SelectedItemData.ItemName.ToString(),
		*SelectedItemData.ItemBaseType.ToString(),
		*SelectedItemData.ItemSlot.ToString(),
		*SelectedItemData.ItemClass.ToString(),
		SelectedItemData.ItemTags.HasTags ? TEXT("true") : TEXT("false"),
		SelectedItemData.ItemAffixes.MaxImplicitMods,
		SelectedItemData.ItemAffixes.MaxPrefixMods,
		SelectedItemData.ItemAffixes.MaxSuffixMods);

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


