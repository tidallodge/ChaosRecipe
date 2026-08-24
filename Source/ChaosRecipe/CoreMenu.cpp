// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "BaseItemStruct.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
    ValidateButton(Item1);
    ValidateButton(Item2);
    ValidateButton(Item3);
    ValidateButton(Item4);
    ValidateButton(ItemInfoButton);
    ValidateButton(RandomizeButton);
    ValidateButton(SaveItemButton);
    ValidateButton(LoadItemButton);

	BuyButton->OnClicked.AddDynamic(this, &UCoreMenu::OnBuyButtonClicked);
	SellButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSellButtonClicked);
	Item1->OnClicked.AddDynamic(this, &UCoreMenu::OnSelectSwordButtonClicked);
	Item2->OnClicked.AddDynamic(this, &UCoreMenu::OnSelectAxeButtonClicked);
	Item3->OnClicked.AddDynamic(this, &UCoreMenu::OnSelectShieldButtonClicked);
	Item4->OnClicked.AddDynamic(this, &UCoreMenu::OnSelectHatchetButtonClicked);
	ItemInfoButton->OnClicked.AddDynamic(this, &UCoreMenu::OnItemInfoButtonClicked);
	RandomizeButton->OnClicked.AddDynamic(this, &UCoreMenu::OnRandomizeItemButtonClicked);
	SaveItemButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSaveItemButtonClicked);
	LoadItemButton->OnClicked.AddDynamic(this, &UCoreMenu::OnLoadItemButtonClicked);

	PlayerSwordCount = 1;
	PlayerMoneyCount = 20;

	UE_LOG(LogTemp, Warning, TEXT("CoreMenu initialized."));
	UpdateSwordCount(PlayerSwordCount);
	UpdatePlayerMoney(PlayerMoneyCount);

	if (Item1_Icon)
	{
		UTexture2D* SwordTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ItemAssets/WeaponShieldAssets/CutlassTexture2D.CutlassTexture2D"));
		if (SwordTexture)
		{
			Item1_Icon->SetBrushFromTexture(SwordTexture);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load Sword texture for Image1."));
		}
	}

	if (Item2_Icon)
	{
		UTexture2D* HatchetTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ItemAssets/WeaponShieldAssets/BattleAxeTexture2D.BattleAxeTexture2D"));
		if (HatchetTexture)
		{
			Item2_Icon->SetBrushFromTexture(HatchetTexture);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load Hatchet texture for Image2."));
		}
	}

	if (Item3_Icon)
	{
		UTexture2D* ShieldTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ItemAssets/WeaponShieldAssets/BucklerTexture2D.BucklerTexture2D"));
		if (ShieldTexture)
		{
			Item3_Icon->SetBrushFromTexture(ShieldTexture);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load Shield texture for Image3."));
		}
	}

	if (Item4_Icon)
	{
		UTexture2D* HatchetTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ItemAssets/WeaponShieldAssets/Hatchet_Icon_test.Hatchet_Icon_test"));
		if (HatchetTexture)
		{
			Item4_Icon->SetBrushFromTexture(HatchetTexture);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load Hatchet texture for Image4."));
		}
	}
}

void UCoreMenu::OnSellButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("SellButton Clicked."));
	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item to sell."));
		return;
	}
	OnSellButtonClickedEvent.Broadcast(SelectedItemId);
}

void UCoreMenu::OnBuyButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("BuyButton Clicked. Event Dispatched"));
	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item to buy."));
		return;
	}
	OnBuyButtonClickedEvent.Broadcast(SelectedItemId);
}

void UCoreMenu::OnSelectSwordButtonClicked()
{
	SelectItemData(FText::FromString(TEXT("sword_1h_001")));
}

void UCoreMenu::OnSelectAxeButtonClicked()
{
	SelectItemData(FText::FromString(TEXT("axe_2h_001")));
}

void UCoreMenu::OnSelectShieldButtonClicked()
{
	SelectItemData(FText::FromString(TEXT("shield_1h_001")));
}

void UCoreMenu::OnSelectHatchetButtonClicked()
{
	SelectItemData(FText::FromString(TEXT("axe_1h_001")));
}

void UCoreMenu::SelectItemData(const FText& ItemIdText)
{
	UDataTable* ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ItemData/BaseItem_DT"));
	if (!ItemDataTable)
	{
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
			SelectedItemId = ItemRow->ItemId.ToString();
			bHasSelectedItemData = true;

			FString ItemClassName = UEnum::GetValueAsString(ItemRow->ItemClass);
			FString ItemSlotName = UEnum::GetValueAsString(ItemRow->ItemSlot);
			FString ItemIconName = ItemRow->ItemAssetData.ItemIcon ? ItemRow->ItemAssetData.ItemIcon->GetName() : TEXT("None");
			FString ItemStaticMeshName = ItemRow->ItemAssetData.ItemStaticMesh ? ItemRow->ItemAssetData.ItemStaticMesh->GetName() : TEXT("None");

			FString ItemInfo = FString::Printf(
				TEXT("Item ID: %s\nName: %s\nClass: %s\nSlot: %s\nIcon: %s\nStaticMesh: %s"),
				*ItemRow->ItemId.ToString(),
				*ItemRow->ItemName.ToString(),
				*ItemClassName,
				*ItemSlotName,
				*ItemIconName,
				*ItemStaticMeshName);
			LogToScreen(ItemInfo);
			UE_LOG(LogTemp, Warning, TEXT("Selected item data: %s"), *ItemInfo);
			return;
		}
	}

	bHasSelectedItemData = false;
	UE_LOG(LogTemp, Warning, TEXT("No item found for ItemId: %s"), *SearchText);
}

void UCoreMenu::OnItemInfoButtonClicked()
{
	if (!bHasSelectedItemData)
	{
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

	UE_LOG(LogTemp, Warning, TEXT("Item info: %s"), *ItemInfo);
	FString ItemId = FString(SelectedItemData.ItemId.ToString());
	LogToScreen(ItemInfo);
	OnItemInfoButtonClickedEvent.Broadcast(ItemId);
}

void UCoreMenu::OnRandomizeItemButtonClicked()
{
	const float RandomValue = FMath::FRandRange(0.5f, 2.0f);
	UE_LOG(LogTemp, Warning, TEXT("RandomizeItemButton Clicked. RandomValue: %.2f"), RandomValue);
	OnRandomizeItemEvent.Broadcast(RandomValue);
}

void UCoreMenu::OnSaveItemButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("SaveItemButton Clicked."));
	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item to save."));
		return;
	}
	OnSaveItemButtonClickedEvent.Broadcast(SelectedItemId);
}

static TArray<FString> GetAllSavedItemUUIDs()
{
	TArray<FString> OutUUIDs;
	const FString SaveFilePath = FPaths::ProjectSavedDir() / TEXT("SavedItems.json");

	FString InputString;
	if (!FFileHelper::LoadFileToString(InputString, *SaveFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load SavedItems.json at %s"), *SaveFilePath);
		return OutUUIDs;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse SavedItems.json"));
		return OutUUIDs;
	}

	const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
	if (!RootObject->TryGetArrayField(TEXT("Items"), ItemsArray))
	{
		UE_LOG(LogTemp, Warning, TEXT("SavedItems.json has no items."));
		return OutUUIDs;
	}

	for (const TSharedPtr<FJsonValue>& ItemValue : *ItemsArray)
	{
		const TSharedPtr<FJsonObject>* ItemObject;
		FString ItemUUID;
		if (ItemValue->TryGetObject(ItemObject) && (*ItemObject)->TryGetStringField(TEXT("UUID"), ItemUUID) && !ItemUUID.IsEmpty())
		{
			OutUUIDs.Add(ItemUUID);
		}
	}

	return OutUUIDs;
}

void UCoreMenu::OnLoadItemButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("LoadItemButton Clicked."));

	if (!LoadItemVertBox)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadItemVertBox is null or not found!"));
		return;
	}

	LoadItemVertBox->ClearChildren();

	const TArray<FString> SavedUUIDs = GetAllSavedItemUUIDs();
	if (SavedUUIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No saved item UUIDs found."));
		return;
	}

	UClass* SingleButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleButton.WBP_SingleButton_C"));
	if (!SingleButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleButton class."));
		return;
	}

	for (const FString& UUID : SavedUUIDs)
	{
		UUserWidget* NewSingleButton = CreateWidget<UUserWidget>(this, SingleButtonClass);
		if (!NewSingleButton)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create WBP_SingleButton widget instance."));
			continue;
		}

		if (FObjectProperty* TextBlockProp = FindFProperty<FObjectProperty>(NewSingleButton->GetClass(), TEXT("SingleButtonText")))
		{
			if (UTextBlock* SingleButtonTextBlock = Cast<UTextBlock>(TextBlockProp->GetPropertyValue_InContainer(NewSingleButton)))
			{
				SingleButtonTextBlock->SetText(FText::FromString(UUID));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SingleButtonText resolved to a null/non-TextBlock widget."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleButtonText property not found on WBP_SingleButton."));
		}

		LoadItemVertBox->AddChildToVerticalBox(NewSingleButton);
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

void UCoreMenu::LogToScreen(const FString& NewMessage)
{
	if (TestTextBlock)
	{
		TestTextBlock->SetText(FText::FromString(NewMessage));
	}
}


