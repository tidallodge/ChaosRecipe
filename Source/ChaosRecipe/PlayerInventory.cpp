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

	ItemCountById.Empty();
	ItemCountById.Add(TEXT("Sword"), 1);

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
	int32* ItemCount = ItemCountById.Find(ItemType);
	if (ItemCount && *ItemCount > 0)
	{
		ValidSale = 1;
		*ItemCount -= 1;
	}
	else
	{
		ValidSale = 0;
	}

	if (!ValidSale)
	{
		UE_LOG(LogTemp, Warning, TEXT("No item to sell"));
	}

	if (CoreMenuRef)
	{
		int32* SwordCountPtr = ItemCountById.Find(TEXT("Sword"));
		int32 SwordCount = SwordCountPtr ? *SwordCountPtr : 0;
		CoreMenuRef->UpdateSwordCount(SwordCount);
	}
}

void UPlayerInventory::HandleStoreBuy(FString ItemType, int32 ItemValue)
{
	ItemCountById.FindOrAdd(ItemType) += 1;

	if (CoreMenuRef)
	{
		int32* SwordCountPtr = ItemCountById.Find(TEXT("Sword"));
		int32 SwordCount = SwordCountPtr ? *SwordCountPtr : 0;
		CoreMenuRef->UpdateSwordCount(SwordCount);
	}
}