// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/ButtonSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "BaseItemStruct.h"
#include "ItemInstanceManager.h"
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

	ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ItemData/BaseItem_DT"));
	if (ItemDataTable)
	{
		ItemDataTableRowNames = ItemDataTable->GetRowNames();
		ItemDataTableRowCount = ItemDataTable->GetRowMap().Num();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load BaseItem_DT data table."));
	}


    ValidateButton(BuyButton);
    ValidateButton(SellButton);
    ValidateButton(RandomizeButton);
    ValidateButton(SaveItemButton);
    ValidateButton(ShopButton);
    ValidateButton(RandomizeShopButton);
    ValidateButton(PlayerStashButton);
    ValidateButton(StashSelectButton);

	BuyButton->OnClicked.AddDynamic(this, &UCoreMenu::OnBuyButtonClicked);
	SellButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSellButtonClicked);
	RandomizeButton->OnClicked.AddDynamic(this, &UCoreMenu::OnRandomizeItemButtonClicked);
	SaveItemButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSaveItemButtonClicked);
	ShopButton->OnClicked.AddDynamic(this, &UCoreMenu::OnShopButtonClicked);
	RandomizeShopButton->OnClicked.AddDynamic(this, &UCoreMenu::OnRandomizeShopButtonClicked);
	PlayerStashButton->OnClicked.AddDynamic(this, &UCoreMenu::OnPlayerStashButtonClicked);
	StashSelectButton->OnClicked.AddDynamic(this, &UCoreMenu::OnStashSelectButtonClicked);

	if (ShopWindowBox)
	{
		SetPanelAndChildrenVisibility(ShopWindowBox, ESlateVisibility::Hidden);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ShopWindowBox is null or not found!"));
	}

	PlayerSwordCount = 1;

	UE_LOG(LogTemp, Warning, TEXT("CoreMenu initialized."));
	UpdateSwordCount(PlayerSwordCount);

	PopulatePlayerStash();

	RandomizeShopItems();
	PopulateShopGrid();
}

void UCoreMenu::OnSellButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("SellButton Clicked."));
	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item to sell."));
		return;
	}
	OnSellButtonClickedEvent.Broadcast(SelectedItemId, SelectedItemUUID);
}

void UCoreMenu::OnBuyButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("BuyButton Clicked. Event Dispatched"));
	// Unlike Sell, Buy keeps ShopWindowBox open so the player can keep shopping, and leaves
	// ActiveItemImageHorizBox alone rather than showing the purchased item there.
	UpdatePanelVisibility({ PlayerStashHorizBox }, ESlateVisibility::Hidden);
	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item to buy."));
		return;
	}
	OnBuyButtonClickedEvent.Broadcast(SelectedItemId, SelectedItemUUID);

	// Remove the purchased listing from the shop's current item list so it's no longer offered,
	// then rebuild the grid from the reduced list. Matches on ItemId rather than array index so a
	// duplicate listing elsewhere in the shop isn't accidentally removed instead.
	if (ItemDataTable)
	{
		for (int32 Index = 0; Index < CurrentShopItems.Num(); ++Index)
		{
			const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(CurrentShopItems[Index], TEXT("CoreMenu::OnBuyButtonClicked"));
			if (ItemRow && ItemRow->ItemId.ToString().Equals(SelectedItemId, ESearchCase::IgnoreCase))
			{
				CurrentShopItems.RemoveAt(Index);
				break;
			}
		}
	}

	RefreshShopGrid();
}

void UCoreMenu::SelectItemData(const FText& ItemIdText)
{
	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseItem_DT data table is not loaded."));
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
			SetActiveItemText(ItemInfo);
			UE_LOG(LogTemp, Warning, TEXT("Selected item data: %s"), *ItemInfo);
			return;
		}
	}

	bHasSelectedItemData = false;
	UE_LOG(LogTemp, Warning, TEXT("No item found for ItemId: %s"), *SearchText);
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

	// Marks this stash item as the active item, so RandomizeButton/SaveItemButton act on it.
	SelectItemData(FText::FromString(ItemId));

	// Lets listeners (e.g. ItemHandler) load this existing saved item's stats into their
	// working cache, keyed to its already-assigned UUID, so Randomize/Save edit it in place
	// instead of a stale or nonexistent cached item.
	OnStashItemSelectedEvent.Broadcast(ItemId, ItemUUID);

	// Item display name comes from BaseItem_DT (the saved JSON only stores the ItemId).
	const FString ItemName = bHasSelectedItemData ? SelectedItemData.ItemName.ToString() : ItemId;

	double AttackRate = 0.0;
	ItemObject->TryGetNumberField(TEXT("attackRate"), AttackRate);

	// itemGoldValue is populated as soon as an item's stats are built (it starts equal to
	// itemBaseGoldValue), so the itemBaseGoldValue fallback only matters for saves from before
	// this field existed.
	double ItemGoldValue = 0.0;
	double ItemBaseGoldValue = 0.0;
	ItemObject->TryGetNumberField(TEXT("itemGoldValue"), ItemGoldValue);
	ItemObject->TryGetNumberField(TEXT("itemBaseGoldValue"), ItemBaseGoldValue);
	const double DisplayGoldValue = ItemGoldValue > 0.0 ? ItemGoldValue : ItemBaseGoldValue;

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
		TEXT("%s\nBase Damage:\n%sAttack Rate: %.2f\nGold Value: %.0f\n%s"),
		*ItemName,
		*WeaponDamageText,
		AttackRate,
		DisplayGoldValue,
		*ModifiersText);

	ActiveItemTextBox->SetText(FText::FromString(DisplayText));
}

void UCoreMenu::PopulatePlayerStash()
{
	if (!PlayerStashUniGrid)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerStashUniGrid is null or not found!"));
		return;
	}

	PlayerStashUniGrid->ClearChildren();
	PlayerStashUniGrid->SetSlotPadding(FMargin(4.f));
	PlayerStashButtonProxies.Empty();

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseItem_DT data table is not loaded."));
		return;
	}

	UClass* SingleImageButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleImageButton.WBP_SingleImageButton_C"));
	if (!SingleImageButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleImageButton class."));
		return;
	}

	constexpr int32 NumColumns = 4;
	constexpr float StashItemSlotSize = 256.f;
	int32 Index = 0;

	UItemInstanceManager StashManager;

	for (const TPair<FString, TSharedPtr<FJsonObject>>& SavedItem : StashManager.GetSavedItems())
	{
		FString ItemId;
		if (SavedItem.Value.IsValid())
		{
			SavedItem.Value->TryGetStringField(TEXT("ItemId"), ItemId);
		}

		const FBaseItemStruct* ItemRow = nullptr;
		for (const FName& RowName : ItemDataTable->GetRowNames())
		{
			const FBaseItemStruct* CandidateRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("CoreMenu::PopulatePlayerStash"));
			if (CandidateRow && CandidateRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
			{
				ItemRow = CandidateRow;
				break;
			}
		}

		if (!ItemRow)
		{
			UE_LOG(LogTemp, Warning, TEXT("No BaseItem_DT row found for saved ItemId: %s"), *ItemId);
			continue;
		}

		UUserWidget* StashItemWidget = CreateWidget<UUserWidget>(this, SingleImageButtonClass);
		USizeBox* ItemSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (!StashItemWidget || !ItemSlotBox)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to construct WBP_SingleImageButton/SizeBox for stash item."));
			continue;
		}

		// Forcing the size here (rather than on the icon alone) guarantees every
		// uniform grid cell is exactly StashItemSlotSize, since UUniformGridPanel
		// sizes all cells to match the largest child added to the grid.
		ItemSlotBox->SetWidthOverride(StashItemSlotSize);
		ItemSlotBox->SetHeightOverride(StashItemSlotSize);

		if (FObjectProperty* IconProp = FindFProperty<FObjectProperty>(StashItemWidget->GetClass(), TEXT("SingleImageButtonIcon")))
		{
			if (UImage* SingleImageButtonIcon = Cast<UImage>(IconProp->GetPropertyValue_InContainer(StashItemWidget)))
			{
				if (ItemRow->ItemAssetData.ItemIcon)
				{
					SingleImageButtonIcon->SetBrushFromTexture(ItemRow->ItemAssetData.ItemIcon);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("No ItemIcon set for item: %s"), *ItemRow->ItemId.ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SingleImageButtonIcon resolved to a null/non-Image widget."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleImageButtonIcon property not found on WBP_SingleImageButton."));
		}

		UButton* InnerButton = nullptr;
		if (FObjectProperty* ButtonProp = FindFProperty<FObjectProperty>(StashItemWidget->GetClass(), TEXT("SingleImageButton")))
		{
			InnerButton = Cast<UButton>(ButtonProp->GetPropertyValue_InContainer(StashItemWidget));
		}
		if (!InnerButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleImageButton property not found on WBP_SingleImageButton."));
			continue;
		}

		ItemSlotBox->AddChild(StashItemWidget);

		ULoadItemButtonProxy* Proxy = NewObject<ULoadItemButtonProxy>(this);
		Proxy->ItemUUID = SavedItem.Key;
		Proxy->OwningMenu = this;
		PlayerStashButtonProxies.Add(Proxy);
		InnerButton->OnClicked.AddDynamic(Proxy, &ULoadItemButtonProxy::HandleClicked);

		PlayerStashUniGrid->AddChildToUniformGrid(ItemSlotBox, Index / NumColumns, Index % NumColumns);

		++Index;
	}
}

void UShopItemButtonProxy::HandleClicked()
{
	if (OwningMenu)
	{
		OwningMenu->OnShopItemButtonClicked(ItemId, ItemUUID);
	}
}

void UCoreMenu::OnShopButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("ShopButton Clicked."));

	if (!ShopWindowBox)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopWindowBox is null or not found!"));
		return;
	}

	const bool bShouldShow = ShopWindowBox->GetVisibility() != ESlateVisibility::Visible;
	const ESlateVisibility NewVisibility = bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	SetPanelAndChildrenVisibility(ShopWindowBox, NewVisibility);

	if (PlayerStashHorizBox)
	{
		SetPanelAndChildrenVisibility(PlayerStashHorizBox, ESlateVisibility::Hidden);
	}

	UpdatePanelVisibility({ ActiveItemImageHorizBox }, ESlateVisibility::Hidden);

	if (bShouldShow)
	{
		PopulateShopGrid();
	}
}

void UCoreMenu::OnRandomizeShopButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("RandomizeShopButton Clicked."));
	RandomizeShopItems();
}

void UCoreMenu::PopulateShopGrid()
{
	if (!ShopUniGrid)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopUniGrid is null or not found!"));
		return;
	}

	ShopUniGrid->ClearChildren();
	ShopUniGrid->SetSlotPadding(FMargin(4.f));
	ShopItemButtonProxies.Empty();

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseItem_DT data table is not loaded."));
		return;
	}

	UClass* SingleImageButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleImageButton.WBP_SingleImageButton_C"));
	if (!SingleImageButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleImageButton class."));
		return;
	}

	constexpr int32 NumColumns = 4;
	constexpr float ShopItemSlotSize = 256.f;
	int32 Index = 0;

	for (const FName& RowName : CurrentShopItems)
	{
		const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("CoreMenu::PopulateShopGrid"));
		if (!ItemRow)
		{
			continue;
		}

		UUserWidget* ShopItemWidget = CreateWidget<UUserWidget>(this, SingleImageButtonClass);
		USizeBox* ItemSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (!ShopItemWidget || !ItemSlotBox)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to construct WBP_SingleImageButton/SizeBox for shop item."));
			continue;
		}

		// Forcing the size here (rather than on the icon alone) guarantees every
		// uniform grid cell is exactly ShopItemSlotSize, since UUniformGridPanel
		// sizes all cells to match the largest child added to the grid.
		ItemSlotBox->SetWidthOverride(ShopItemSlotSize);
		ItemSlotBox->SetHeightOverride(ShopItemSlotSize);

		if (FObjectProperty* IconProp = FindFProperty<FObjectProperty>(ShopItemWidget->GetClass(), TEXT("SingleImageButtonIcon")))
		{
			if (UImage* SingleImageButtonIcon = Cast<UImage>(IconProp->GetPropertyValue_InContainer(ShopItemWidget)))
			{
				if (ItemRow->ItemAssetData.ItemIcon)
				{
					SingleImageButtonIcon->SetBrushFromTexture(ItemRow->ItemAssetData.ItemIcon);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("No ItemIcon set for item: %s"), *ItemRow->ItemId.ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SingleImageButtonIcon resolved to a null/non-Image widget."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleImageButtonIcon property not found on WBP_SingleImageButton."));
		}

		UButton* InnerButton = nullptr;
		if (FObjectProperty* ButtonProp = FindFProperty<FObjectProperty>(ShopItemWidget->GetClass(), TEXT("SingleImageButton")))
		{
			InnerButton = Cast<UButton>(ButtonProp->GetPropertyValue_InContainer(ShopItemWidget));
		}
		if (!InnerButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleImageButton property not found on WBP_SingleImageButton."));
			continue;
		}

		ItemSlotBox->AddChild(ShopItemWidget);

		UShopItemButtonProxy* Proxy = NewObject<UShopItemButtonProxy>(this);
		Proxy->ItemId = ItemRow->ItemId.ToString();
		// Mint this shop listing's UUID now, rather than waiting for Buy to be clicked, so every
		// item shown in the grid already has one assigned the moment it's populated.
		Proxy->ItemUUID = FGuid::NewGuid().ToString();
		Proxy->OwningMenu = this;
		ShopItemButtonProxies.Add(Proxy);
		InnerButton->OnClicked.AddDynamic(Proxy, &UShopItemButtonProxy::HandleClicked);

		ShopUniGrid->AddChildToUniformGrid(ItemSlotBox, Index / NumColumns, Index % NumColumns);

		++Index;
	}
}

void UCoreMenu::RefreshShopGrid()
{
	PopulateShopGrid();
}

void UCoreMenu::OnShopItemButtonClicked(FString ItemId, FString ItemUUID)
{
	UE_LOG(LogTemp, Warning, TEXT("Shop item button clicked for ItemId: %s (UUID: %s)"), *ItemId, *ItemUUID);
	SelectItemData(FText::FromString(ItemId));
	SelectedItemUUID = ItemUUID;

	// Lets listeners (e.g. ItemHandler) cache this item's base stats and display them as the
	// active item, matching the stat breakdown shown for a stash selection or randomize.
	OnShopItemSelectedEvent.Broadcast(ItemId, ItemUUID);
}

void UCoreMenu::OnPlayerStashButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerStashButton Clicked."));

	if (!PlayerStashHorizBox)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerStashHorizBox is null or not found!"));
		return;
	}

	SetPanelAndChildrenVisibility(PlayerStashHorizBox, ESlateVisibility::Visible);

	if (ShopWindowBox)
	{
		SetPanelAndChildrenVisibility(ShopWindowBox, ESlateVisibility::Hidden);
	}

	UpdatePanelVisibility({ ActiveItemImageHorizBox }, ESlateVisibility::Hidden);

	PopulatePlayerStash();
}

void UCoreMenu::OnStashSelectButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("StashSelectButton Clicked."));
	UpdatePanelVisibility({ ShopWindowBox, PlayerStashHorizBox }, ESlateVisibility::Hidden);
	ShowActiveItemImage();
}

void UCoreMenu::UpdatePanelVisibility(const TArray<UPanelWidget*>& Panels, ESlateVisibility NewVisibility)
{
	for (UPanelWidget* Panel : Panels)
	{
		SetPanelAndChildrenVisibility(Panel, NewVisibility);
	}
}

void UCoreMenu::ShowActiveItemImage()
{
	if (ActiveItemImageHorizBox)
	{
		SetPanelAndChildrenVisibility(ActiveItemImageHorizBox, ESlateVisibility::Visible);
	}

	if (!ActiveItemImage)
	{
		return;
	}

	if (bHasSelectedItemData && SelectedItemData.ItemAssetData.ItemIcon)
	{
		ActiveItemImage->SetBrushFromTexture(SelectedItemData.ItemAssetData.ItemIcon);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item icon available for ActiveItemImage."));
	}
}

void UCoreMenu::SetPanelAndChildrenVisibility(UPanelWidget* Panel, ESlateVisibility NewVisibility)
{
	if (!Panel)
	{
		return;
	}

	Panel->SetVisibility(NewVisibility);
	for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
	{
		if (UWidget* Child = Panel->GetChildAt(i))
		{
			Child->SetVisibility(NewVisibility);
		}
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

void UCoreMenu::RandomizeShopItems()
{
	CurrentShopItems.Empty();

	ShopItemCount = FMath::RandRange(6,12);

	for (int32 i = 0; i < ShopItemCount; i++)
	{
		int32 RandomItemRowName = FMath::RandRange(1, ItemDataTableRowCount);
		RandomItemRowName -= 1;
		CurrentShopItems.Add(ItemDataTableRowNames[RandomItemRowName]);
	}

	FString ShopItemsLog;
	for (const FName& RowName : CurrentShopItems)
	{
		ShopItemsLog += RowName.ToString() + TEXT("\n");
	}
	LogToScreen(ShopItemsLog);

	PopulateShopGrid();
}

void UCoreMenu::UpdateSwordCount(int32 PlayerSwords)
{
}

void UCoreMenu::LogToScreen(const FString& NewMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *NewMessage);
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

void UCoreMenu::SetPlayerGoldText(int32 NewGoldCount)
{
	if (PlayerGoldTextBox)
	{
		PlayerGoldTextBox->SetText(FText::AsNumber(NewGoldCount));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerGoldTextBox is null or not found!"));
	}
}


