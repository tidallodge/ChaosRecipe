// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerInventory.h"
#include "CoreMenu.h"
#include "StoreManager.h"
#include "ItemHandler.h"
#include "CurrencyManager.h"
#include "Kismet/KismetSystemLibrary.h"

UPlayerInventory::UPlayerInventory()
{
	CurrencyManager = MakeUnique<UCurrencyManager>();
	CurrencyManager->LoadPlayerGoldCount(PlayerGoldCount);
}

UPlayerInventory::UPlayerInventory(FVTableHelper& Helper) : Super(Helper)
{
}

UPlayerInventory::~UPlayerInventory() = default;

void UPlayerInventory::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{
	if (!CoreMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("no CoreMenu for PlayerInventory"));
		return;
	}

	BoundCoreMenu = CoreMenu;
	CoreMenu->OnResetGameButtonClickedEvent.AddDynamic(this, &UPlayerInventory::OnResetGame);

	// Reflects the gold count loaded from SavedCurrency.json (in the constructor) now that a
	// CoreMenu is available to display it.
	UpdatePlayerGoldDisplay();
}

void UPlayerInventory::BindToItemHandlerEvents(UItemHandler* ItemHandler)
{
	if (!ItemHandler)
	{
		UE_LOG(LogTemp, Error, TEXT("no ItemHandler for PlayerInventory"));
		return;
	}

	ItemHandler->OnItemSoldEvent.AddDynamic(this, &UPlayerInventory::HandleItemSold);
	ItemHandler->OnItemBoughtEvent.AddDynamic(this, &UPlayerInventory::HandleItemBought);
	ItemHandler->OnItemRandomizedEvent.AddDynamic(this, &UPlayerInventory::HandleItemRandomized);
}

void UPlayerInventory::BindToStoreManagerEvents(UStoreManager* StoreManager)
{
	if (!StoreManager)
	{
		UE_LOG(LogTemp, Error, TEXT("no StoreManager for PlayerInventory"));
		return;
	}

	StoreManager->OnStoreBuy.AddDynamic(this, &UPlayerInventory::HandleStoreBuy);
}

void UPlayerInventory::HandleStoreBuy(FString ItemType, FString ItemUUID)
{
	UE_LOG(LogTemp, Warning, TEXT("HandleStoreBuy: ItemId=%s, UUID=%s"), *ItemType, *ItemUUID);
}

void UPlayerInventory::HandleItemSold(FString ItemId, FString ItemUUID, float GoldValue)
{
	const int32 RoundedGoldValue = FMath::RoundToInt(GoldValue);
	AdjustPlayerGoldCount(RoundedGoldValue);

	UE_LOG(LogTemp, Warning, TEXT("PlayerInventory: Sold %s (UUID: %s) for %d gold (PlayerGoldCount=%d)"),
		*ItemId, *ItemUUID, RoundedGoldValue, PlayerGoldCount);
}

void UPlayerInventory::HandleItemBought(FString ItemId, FString ItemUUID, float GoldValue)
{
	const int32 RoundedGoldValue = FMath::RoundToInt(GoldValue);
	AdjustPlayerGoldCount(-RoundedGoldValue);

	UE_LOG(LogTemp, Warning, TEXT("PlayerInventory: Bought %s (UUID: %s) for %d gold (PlayerGoldCount=%d)"),
		*ItemId, *ItemUUID, RoundedGoldValue, PlayerGoldCount);
}

void UPlayerInventory::HandleItemRandomized(int32 GoldCost)
{
	AdjustPlayerGoldCount(-GoldCost);

	UE_LOG(LogTemp, Warning, TEXT("PlayerInventory: Randomized item for %d gold (PlayerGoldCount=%d)"),
		GoldCost, PlayerGoldCount);
}

void UPlayerInventory::OnResetGame()
{
	CurrencyManager->DeleteSavedCurrency();

	PlayerGoldCount = DefaultPlayerGoldCount;
	CurrencyManager->SaveCurrency(PlayerGoldCount);
	UpdatePlayerGoldDisplay();

	UE_LOG(LogTemp, Warning, TEXT("PlayerInventory: Reset - PlayerGoldCount back to default (%d)"), PlayerGoldCount);
}

bool UPlayerInventory::CanAffordGoldCost(int32 GoldCost) const
{
	int32 AdjustValue = GoldCost;
	int32 CurrentCurrency = PlayerGoldCount;
	return CurrencyManager->ValidateCurrencyUpdate(AdjustValue, CurrentCurrency);
}

void UPlayerInventory::AdjustPlayerGoldCount(int32 GoldDelta)
{
	PlayerGoldCount += GoldDelta;

	CurrencyManager->SaveCurrency(PlayerGoldCount);
	UpdatePlayerGoldDisplay();
}

void UPlayerInventory::UpdatePlayerGoldDisplay() const
{
	if (BoundCoreMenu)
	{
		BoundCoreMenu->SetPlayerGoldText(PlayerGoldCount);
	}
}