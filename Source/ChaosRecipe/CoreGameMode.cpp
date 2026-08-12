// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "CoreMenu.h"
#include "PlayerInventory.h"
#include "StoreManager.h"
#include "ItemHandler.h"

void ACoreGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Load and display the CoreMenu widget
	if (!CoreMenuClass)
	{
		// Try to load WBP_CoreMenu Blueprint widget (Blueprint class path uses _C suffix)
		CoreMenuClass = LoadClass<UCoreMenu>(nullptr, TEXT("/Game/WBP_CoreMenu"));
	}

	if (CoreMenuClass)
	{
		CoreMenuWidget = CreateWidget<UCoreMenu>(GetWorld(), CoreMenuClass);
		if (CoreMenuWidget)
		{
			CoreMenuWidget->AddToViewport();

			UPlayerInventory* PlayerInventory = NewObject<UPlayerInventory>(this);
			PlayerInventory->BindToCoreMenuEvents(CoreMenuWidget);

			ItemHandler = NewObject<UItemHandler>(this);
			if (ItemHandler)
			{
				ItemHandler->BindToCoreMenuEvents(CoreMenuWidget);
			}

			UStoreManager* StoreManager = NewObject<UStoreManager>(this);
			StoreManager->BindToCoreMenuEvents(CoreMenuWidget);
			PlayerInventory->BindToStoreManagerEvents(StoreManager);

			UE_LOG(LogTemp, Warning, TEXT("CoreMenu widget loaded and displayed"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create CoreMenu widget"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CoreMenuClass not assigned. Please assign WBP_CoreMenu in CoreGameMode defaults or blueprint."));
	}
}

