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

    CoreMenu->OnBuyButtonClickedEvent.AddDynamic(this, &UStoreManager::OnStoreBuyEvent);
}

void UStoreManager::OnStoreBuyEvent(FString ItemType, FString ItemUUID)
{
    UE_LOG(LogTemp, Warning, TEXT("Broadcasting sale from StoreManager."));
    OnStoreBuy.Broadcast(ItemType, ItemUUID);
}