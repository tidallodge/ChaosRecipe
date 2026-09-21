// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "CoreMenu.h"
#include "PlayerInventory.h"
#include "StoreManager.h"
#include "ItemHandler.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"

ACoreGameMode::ACoreGameMode()
{
	// FObjectFinder/FClassFinder only work inside a constructor - they populate the CDO's
	// UPROPERTY directly, which is what makes the referenced asset a genuine hard reference
	// the cooker will follow (see the "Hard References" comment in CoreGameMode.h).
	static ConstructorHelpers::FClassFinder<UCoreMenu> CoreMenuFinder(TEXT("/Game/WBP_CoreMenu"));
	if (CoreMenuFinder.Succeeded())
	{
		CoreMenuClass = CoreMenuFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> SingleImageButtonFinder(TEXT("/Game/WBP_SingleImageButton"));
	if (SingleImageButtonFinder.Succeeded())
	{
		SingleImageButtonClass = SingleImageButtonFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> BaseItemDataTableFinder(TEXT("/Game/ItemData/BaseItem_DT"));
	if (BaseItemDataTableFinder.Succeeded())
	{
		BaseItemDataTable = BaseItemDataTableFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> BaseWeaponDataTableFinder(TEXT("/Game/ItemData/BaseWeapon_DT"));
	if (BaseWeaponDataTableFinder.Succeeded())
	{
		BaseWeaponDataTable = BaseWeaponDataTableFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> BaseArmorDataTableFinder(TEXT("/Game/ItemData/BaseArmor_DT"));
	if (BaseArmorDataTableFinder.Succeeded())
	{
		BaseArmorDataTable = BaseArmorDataTableFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> ItemModifierDataTableFinder(TEXT("/Game/ItemData/ItemModifier_DT"));
	if (ItemModifierDataTableFinder.Succeeded())
	{
		ItemModifierDataTable = ItemModifierDataTableFinder.Object;
	}
}

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

			PlayerInventory = NewObject<UPlayerInventory>(this);
			PlayerInventory->BindToCoreMenuEvents(CoreMenuWidget);

			ItemHandler = NewObject<UItemHandler>(this);
			if (ItemHandler)
			{
				ItemHandler->BindToCoreMenuEvents(CoreMenuWidget);
				PlayerInventory->BindToItemHandlerEvents(ItemHandler);
				ItemHandler->BindToPlayerInventory(PlayerInventory);
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

