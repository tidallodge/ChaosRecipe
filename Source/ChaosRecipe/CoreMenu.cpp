// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
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
	UE_LOG(LogTemp, Warning, TEXT("RandomizeItemButton Clicked."));
	OnRandomizeItemEvent.Broadcast();
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

static bool GetSavedItemJsonByUUID(const FString& UUID, TSharedPtr<FJsonObject>& OutItemObject)
{
	const FString SaveFilePath = FPaths::ProjectSavedDir() / TEXT("SavedItems.json");

	FString InputString;
	if (!FFileHelper::LoadFileToString(InputString, *SaveFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load SavedItems.json at %s"), *SaveFilePath);
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse SavedItems.json"));
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
	if (!RootObject->TryGetArrayField(TEXT("Items"), ItemsArray))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& ItemValue : *ItemsArray)
	{
		const TSharedPtr<FJsonObject>* ItemObject;
		FString ItemUUID;
		if (ItemValue->TryGetObject(ItemObject) && (*ItemObject)->TryGetStringField(TEXT("UUID"), ItemUUID) && ItemUUID.Equals(UUID, ESearchCase::IgnoreCase))
		{
			OutItemObject = *ItemObject;
			return true;
		}
	}

	return false;
}

void ULoadItemButtonProxy::HandleClicked()
{
	if (OwningMenu)
	{
		OwningMenu->OnSingleLoadItemButtonClicked(ItemUUID);
	}
}

void UCoreMenu::OnSingleLoadItemButtonClicked(FString ItemUUID)
{
	UE_LOG(LogTemp, Warning, TEXT("Saved item button clicked for UUID: %s"), *ItemUUID);

	if (!ActiveItemTextBox)
	{
		UE_LOG(LogTemp, Error, TEXT("ActiveItemTextBox is null or not found!"));
		return;
	}

	TSharedPtr<FJsonObject> ItemObject;
	if (!GetSavedItemJsonByUUID(ItemUUID, ItemObject))
	{
		ActiveItemTextBox->SetText(FText::FromString(FString::Printf(TEXT("No saved item found for UUID: %s"), *ItemUUID)));
		return;
	}

	SelectedItemUUID = ItemUUID;

	FString ItemId;
	ItemObject->TryGetStringField(TEXT("ItemId"), ItemId);

	// Item display name comes from BaseItem_DT (the saved JSON only stores the ItemId).
	FString ItemName = ItemId;
	if (UDataTable* ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ItemData/BaseItem_DT")))
	{
		for (const FName& RowName : ItemDataTable->GetRowNames())
		{
			const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("CoreMenu::OnSingleLoadItemButtonClicked"));
			if (ItemRow && ItemRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
			{
				ItemName = ItemRow->ItemName.ToString();
				break;
			}
		}
	}

	double AttackRate = 0.0;
	ItemObject->TryGetNumberField(TEXT("attackRate"), AttackRate);

	FString WeaponDamageText;
	const TSharedPtr<FJsonObject>* WeaponDamageObject;
	if (ItemObject->TryGetObjectField(TEXT("weaponDamage"), WeaponDamageObject))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& DamagePair : (*WeaponDamageObject)->Values)
		{
			const TSharedPtr<FJsonObject>* DamageObject;
			if (DamagePair.Value->TryGetObject(DamageObject))
			{
				int32 MinDamage = 0;
				int32 MaxDamage = 0;
				FString DamageType;
				(*DamageObject)->TryGetNumberField(TEXT("minDamage"), MinDamage);
				(*DamageObject)->TryGetNumberField(TEXT("maxDamage"), MaxDamage);
				(*DamageObject)->TryGetStringField(TEXT("damageType"), DamageType);
				WeaponDamageText += FString::Printf(TEXT("  %s: %d-%d (%s)\n"), *DamagePair.Key, MinDamage, MaxDamage, *DamageType);
			}
		}
	}

	// Collect implicit/prefix/suffix modifiers (ModifierId -> rolled value).
	auto AppendModifiers = [&ItemObject](const TCHAR* FieldName, const TCHAR* Label, FString& OutText)
	{
		const TSharedPtr<FJsonObject>* ModifierObject;
		if (!ItemObject->TryGetObjectField(FieldName, ModifierObject) || (*ModifierObject)->Values.Num() == 0)
		{
			return;
		}

		OutText += FString::Printf(TEXT("%s:\n"), Label);
		for (const TPair<FString, TSharedPtr<FJsonValue>>& ModifierPair : (*ModifierObject)->Values)
		{
			double ModifierValue = 0.0;
			ModifierPair.Value->TryGetNumber(ModifierValue);
			OutText += FString::Printf(TEXT("  %s (%d)\n"), *ModifierPair.Key, FMath::RoundToInt(ModifierValue));
		}
	};

	FString ModifiersText;
	AppendModifiers(TEXT("implicitModifiers"), TEXT("Implicit"), ModifiersText);
	AppendModifiers(TEXT("prefixModifiers"), TEXT("Prefixes"), ModifiersText);
	AppendModifiers(TEXT("suffixModifiers"), TEXT("Suffixes"), ModifiersText);
	if (ModifiersText.IsEmpty())
	{
		ModifiersText = TEXT("Modifiers: none\n");
	}

	const FString DisplayText = FString::Printf(
		TEXT("%s\nBase Damage:\n%sAttack Rate: %.2f\n%s"),
		*ItemName,
		*WeaponDamageText,
		AttackRate,
		*ModifiersText);

	ActiveItemTextBox->SetText(FText::FromString(DisplayText));
}

void UCoreMenu::OnLoadItemButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("LoadItemButton Clicked."));

	if (!LoadItemVertBox)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadItemVertBox is null or not found!"));
		return;
	}

	if (LoadItemHorizBox)
	{
		LoadItemHorizBox->SetVisibility(ESlateVisibility::Visible);
	}

	if (!bCloseLoadItemButtonCreated)
	{
		CreateCloseLoadItemButton();
		bCloseLoadItemButtonCreated = true;
	}

	LoadItemVertBox->ClearChildren();
	LoadItemButtonProxies.Empty();

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

		if (FObjectProperty* ButtonProp = FindFProperty<FObjectProperty>(NewSingleButton->GetClass(), TEXT("SingleButton")))
		{
			if (UButton* InnerButton = Cast<UButton>(ButtonProp->GetPropertyValue_InContainer(NewSingleButton)))
			{
				ULoadItemButtonProxy* Proxy = NewObject<ULoadItemButtonProxy>(this);
				Proxy->ItemUUID = UUID;
				Proxy->OwningMenu = this;
				LoadItemButtonProxies.Add(Proxy);

				InnerButton->OnClicked.AddDynamic(Proxy, &ULoadItemButtonProxy::HandleClicked);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SingleButton resolved to a null/non-Button widget."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleButton property not found on WBP_SingleButton."));
		}

		LoadItemVertBox->AddChildToVerticalBox(NewSingleButton);
	}
}

void UCoreMenu::OnCloseLoadItemBoxButtonClicked()
{
	if (!LoadItemHorizBox)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadItemHorizBox is null or not found!"));
		return;
	}

	LoadItemHorizBox->SetVisibility(ESlateVisibility::Collapsed);
}

void UCoreMenu::CreateCloseLoadItemButton()
{
	if (!LoadItemHeaderBox)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadItemHeaderBox is null or not found!"));
		return;
	}

	UClass* SingleButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleButton.WBP_SingleButton_C"));
	if (!SingleButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleButton class."));
		return;
	}

	UUserWidget* CloseButtonWidget = CreateWidget<UUserWidget>(this, SingleButtonClass);
	if (!CloseButtonWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create WBP_SingleButton widget instance."));
		return;
	}

	if (FObjectProperty* TextBlockProp = FindFProperty<FObjectProperty>(CloseButtonWidget->GetClass(), TEXT("SingleButtonText")))
	{
		if (UTextBlock* CloseButtonTextBlock = Cast<UTextBlock>(TextBlockProp->GetPropertyValue_InContainer(CloseButtonWidget)))
		{
			CloseButtonTextBlock->SetText(FText::FromString(TEXT("X")));
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

	if (FObjectProperty* ButtonProp = FindFProperty<FObjectProperty>(CloseButtonWidget->GetClass(), TEXT("SingleButton")))
	{
		if (UButton* InnerButton = Cast<UButton>(ButtonProp->GetPropertyValue_InContainer(CloseButtonWidget)))
		{
			InnerButton->OnClicked.AddDynamic(this, &UCoreMenu::OnCloseLoadItemBoxButtonClicked);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleButton resolved to a null/non-Button widget."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SingleButton property not found on WBP_SingleButton."));
	}

	LoadItemHeaderBox->AddChildToVerticalBox(CloseButtonWidget);
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

void UCoreMenu::SetActiveItemText(const FString& NewMessage)
{
	if (ActiveItemTextBox)
	{
		ActiveItemTextBox->SetText(FText::FromString(NewMessage));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ActiveItemTextBox is null or not found!"));
	}
}


