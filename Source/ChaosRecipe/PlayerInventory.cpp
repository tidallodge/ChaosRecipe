// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerInventory.h"
#include "CoreMenu.h"
#include "StoreManager.h"
#include "Kismet/KismetSystemLibrary.h"

void UPlayerInventory::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{
	if (!CoreMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("no CoreMenu for PlayerInventory"));
		return;
	}

	PlayerMoneyCount = 20;
	PlayerSwordCount = 1; 

	CoreMenuRef = CoreMenu;
}

void UPlayerInventory::BindToStoreManagerEvents(UStoreManager* StoreManager)
{
	if (!StoreManager)
	{
		UE_LOG(LogTemp, Error, TEXT("no StoreManager for PlayerInventory"));
		return;
	}

	StoreManager->OnStoreSale.AddDynamic(this, &UPlayerInventory::HandleStoreSale);
	StoreManager->OnStoreBuy.AddDynamic(this, &UPlayerInventory::HandleStoreBuy);
}



void UPlayerInventory::HandleStoreSale(FString ItemType, int32 ItemValue)
{
	if (ItemType == (TEXT("Sword")))
	{
		if (PlayerSwordCount > 0)
		{
			ValidSale = 1;
			PlayerSwordCount -= 1;
			ItemValue = FMath::FloorToInt(ItemValue * ItemValueModifier);
			PlayerMoneyCount += ItemValue;
		}
		else
		{
			ValidSale = 0;
		}
	}
	
	if (!ValidSale)
	{
		UE_LOG(LogTemp, Warning, TEXT("No item to sell, Bank Balance: %d"), PlayerMoneyCount);
	}

	if (CoreMenuRef)
	{
		CoreMenuRef->UpdateSwordCount(PlayerSwordCount);
		CoreMenuRef->UpdatePlayerMoney(PlayerMoneyCount);
	}
}

void UPlayerInventory::HandleStoreBuy(FString ItemType, int32 ItemValue)
{
	if (ItemType == (TEXT("Sword")))
	{
		if (PlayerMoneyCount >= ItemValue)
		{
			ValidBuy = 1;
			PlayerSwordCount += 1;
			PlayerMoneyCount -= ItemValue;
		}
		else
		{
			ValidBuy = 0;
		}
	}
	
	if (!ValidBuy)
	{
		UE_LOG(LogTemp, Warning, TEXT("No item to sell, Bank Balance: %d"), PlayerMoneyCount);
	}

	if (CoreMenuRef)
	{
		CoreMenuRef->UpdateSwordCount(PlayerSwordCount);
		CoreMenuRef->UpdatePlayerMoney(PlayerMoneyCount);
	}
}