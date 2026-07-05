// Fill out your copyright notice in the Description page of Project Settings.


#include "StoreManager.h"
#include "CoreMenu.h"
#include "Kismet/KismetSystemLibrary.h"

void UStoreManager::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{	
    if (!CoreMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("no CoreMenu for PlayerInventory"));
		return;
	}

    CoreMenu->OnSellButtonClickedEvent.AddDynamic(this, &UStoreManager::OnStoreSaleEvent);
    CoreMenu->OnBuyButtonClickedEvent.AddDynamic(this, &UStoreManager::OnStoreBuyEvent);
}

void UStoreManager::OnStoreSaleEvent(FString ItemType)
{
    UE_LOG(LogTemp, Warning, TEXT("Broadcasting sale from StoreManager."));
    OnStoreSale.Broadcast(ItemType, ItemValue);
}

void UStoreManager::OnStoreBuyEvent(FString ItemType)
{
    UE_LOG(LogTemp, Warning, TEXT("Broadcasting sale from StoreManager."));
    OnStoreBuy.Broadcast(ItemType, ItemValue);
}