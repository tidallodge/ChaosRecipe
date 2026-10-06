// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMenu.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "BaseItemStruct.h"
#include "CurrencyStruct.h"
#include "ItemInstanceManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static bool GetSavedItemJsonByUUID(const FString& UUID, TSharedPtr<FJsonObject>& OutItemObject);

// Resolves the SingleImageButtonIcon image inside a WBP_SingleImageButton instance (null if missing).
static UImage* GetSingleImageButtonIcon(UUserWidget* SingleImageButtonWidget)
{
	if (!SingleImageButtonWidget)
	{
		return nullptr;
	}

	FObjectProperty* IconProp = FindFProperty<FObjectProperty>(SingleImageButtonWidget->GetClass(), TEXT("SingleImageButtonIcon"));
	UImage* Icon = IconProp ? Cast<UImage>(IconProp->GetPropertyValue_InContainer(SingleImageButtonWidget)) : nullptr;
	if (!Icon)
	{
		UE_LOG(LogTemp, Warning, TEXT("SingleImageButtonIcon not found on %s."), *SingleImageButtonWidget->GetName());
	}
	return Icon;
}

// Resolves the SingleImageButton button inside a WBP_SingleImageButton instance (null if missing).
static UButton* GetSingleImageButton(UUserWidget* SingleImageButtonWidget)
{
	if (!SingleImageButtonWidget)
	{
		return nullptr;
	}

	FObjectProperty* ButtonProp = FindFProperty<FObjectProperty>(SingleImageButtonWidget->GetClass(), TEXT("SingleImageButton"));
	UButton* Button = ButtonProp ? Cast<UButton>(ButtonProp->GetPropertyValue_InContainer(SingleImageButtonWidget)) : nullptr;
	if (!Button)
	{
		UE_LOG(LogTemp, Warning, TEXT("SingleImageButton not found on %s."), *SingleImageButtonWidget->GetName());
	}
	return Button;
}

// Strips all padding around the icon in a WBP_SingleImageButton instance: the button style's
// normal/pressed padding, plus the slot padding of the icon and every panel between it and the button.
static void RemoveSingleImageButtonPadding(UUserWidget* SingleImageButtonWidget)
{
	UImage* Icon = GetSingleImageButtonIcon(SingleImageButtonWidget);
	if (!Icon)
	{
		return;
	}

	for (UWidget* Widget = Icon; Widget; Widget = Widget->GetParent())
	{
		UPanelSlot* Slot = Widget->Slot;
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Slot))
		{
			ButtonSlot->SetPadding(FMargin(0.f));
		}
		else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
		{
			OverlaySlot->SetPadding(FMargin(0.f));
		}
		else if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(Slot))
		{
			SizeBoxSlot->SetPadding(FMargin(0.f));
		}
		else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Slot))
		{
			BorderSlot->SetPadding(FMargin(0.f));
		}
		else if (UHorizontalBoxSlot* HorizontalBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
		{
			HorizontalBoxSlot->SetPadding(FMargin(0.f));
		}
		else if (UVerticalBoxSlot* VerticalBoxSlot = Cast<UVerticalBoxSlot>(Slot))
		{
			VerticalBoxSlot->SetPadding(FMargin(0.f));
		}

		if (UButton* Button = Cast<UButton>(Widget))
		{
			FButtonStyle Style = Button->GetStyle();
			Style.SetNormalPadding(FMargin(0.f));
			Style.SetPressedPadding(FMargin(0.f));
			Button->SetStyle(Style);
			break;
		}
	}
}

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

	CurrencyDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ItemData/Currency_DT"));
	if (CurrencyDataTable)
	{
		CurrencyDataTableRowNames = CurrencyDataTable->GetRowNames();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Currency_DT data table."));
	}


    ValidateButton(BuyButton);
    ValidateButton(SellButton);
    ValidateButton(ShopButton);
    ValidateButton(RandomizeShopButton);
    ValidateButton(PlayerStashButton);
    ValidateButton(StashSelectButton);
    ValidateButton(ResetGameButton);

	BuyButton->OnClicked.AddDynamic(this, &UCoreMenu::OnBuyButtonClicked);
	SellButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSellButtonClicked);
	ShopButton->OnClicked.AddDynamic(this, &UCoreMenu::OnShopButtonClicked);
	RandomizeShopButton->OnClicked.AddDynamic(this, &UCoreMenu::OnRandomizeShopButtonClicked);
	PlayerStashButton->OnClicked.AddDynamic(this, &UCoreMenu::OnPlayerStashButtonClicked);
	StashSelectButton->OnClicked.AddDynamic(this, &UCoreMenu::OnStashSelectButtonClicked);
	ResetGameButton->OnClicked.AddDynamic(this, &UCoreMenu::OnResetGameButtonClicked);

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

	RemoveSingleImageButtonPadding(ActiveItemRollButton);
	// No currency is selected yet, so the icon has no texture and would render as a plain white box.
	// Hidden (not Collapsed) keeps the button's layout size; LoadCurrencyIcon reveals it once a brush is set.
	if (UImage* RollIcon = GetSingleImageButtonIcon(ActiveItemRollButton))
	{
		RollIcon->SetVisibility(ESlateVisibility::Hidden);
	}
	if (UButton* RollButton = GetSingleImageButton(ActiveItemRollButton))
	{
		RollButton->OnClicked.AddDynamic(this, &UCoreMenu::OnRandomizeItemButtonClicked);
	}

	PopulatePlayerStash();
	PopulateCurrencyGrids();

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

	// Unload the cached item data now that it's been sold, and clear the Load Item box's text.
	SelectedItemData = FBaseItemStruct();
	SelectedItemId.Empty();
	SelectedItemUUID.Empty();
	bHasSelectedItemData = false;
	SetActiveItemText(TEXT(""));

	UpdatePanelVisibility({ ActiveItemImageHorizBox }, ESlateVisibility::Hidden);

	if (PlayerStashHorizBox)
	{
		SetPanelAndChildrenVisibility(PlayerStashHorizBox, ESlateVisibility::Visible);
	}

	// The broadcast above already removed the sold item from SavedItems.json (synchronously, via
	// ItemHandler::OnSellButtonClicked), so repopulate now the stash grid no longer shows it.
	PopulatePlayerStash();
}

void UCoreMenu::OnBuyButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("BuyButton Clicked. Event Dispatched"));
	// Unlike Sell, Buy keeps ShopWindowBox open so the player can keep shopping, and leaves
	// ActiveItemImageHorizBox alone rather than showing the purchased item there.
	UpdatePanelVisibility({ PlayerStashHorizBox }, ESlateVisibility::Hidden);

	// A selected basic currency listing takes priority over item data. PlayerInventory handles the
	// affordability check and charge; the listing stays in the shop either way.
	if (!SelectedShopCurrencyId.IsEmpty())
	{
		OnBuyCurrencyEvent.Broadcast(SelectedShopCurrencyId, BasicCurrencyShopCost);
		return;
	}

	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item to buy."));
		return;
	}
	// ItemHandler::OnBuyButtonClicked runs synchronously here and rejects the purchase (no stash
	// save, no shop removal) if the player can't afford it, so CoreMenu doesn't remove the listing
	// itself - it only knows the purchase succeeded once ItemHandler calls RemoveItemFromShop back.
	OnBuyButtonClickedEvent.Broadcast(SelectedItemId, SelectedItemUUID);
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

	// Randomize edits the selected item in place, so it must already be a saved/purchased item
	// (present in SavedItems.json) before any of the button's functionality runs.
	TSharedPtr<FJsonObject> ItemObject;
	if (!GetSavedItemJsonByUUID(SelectedItemUUID, ItemObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("RandomizeItemButton: no saved item found for UUID: %s"), *SelectedItemUUID);
		SetWarningText(TEXT("You have not purchased this item yet."));
		return;
	}

	UpdatePanelVisibility({ ShopWindowBox, PlayerStashHorizBox }, ESlateVisibility::Hidden);
	ShowActiveItemImage();

	OnRandomizeItemEvent.Broadcast(SelectedCurrencyId);
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
	SelectedShopCurrencyId.Empty();

	FString ItemId;
	ItemObject->TryGetStringField(TEXT("ItemId"), ItemId);

	// Marks this stash item as the active item, so ActiveItemRollButton acts on it.
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

	// Damage is saved by FJsonObjectConverter as per-channel FIntPoints ({"x": min, "y": max}).
	// Prefer weaponLocalDamage.LocalDamage (base damage with rolled modifiers applied, which is what
	// ItemHandler shows for a freshly rolled item), falling back to weaponDamage.BaseDamage.
	auto BuildDamageLinesFromJson = [](const TSharedPtr<FJsonObject>& DamageObject, const TCHAR* FieldPrefix)
	{
		const TCHAR* Channels[][2] = {
			{ TEXT("Physical"), TEXT("PhysicalDamage") },
			{ TEXT("Fire"),     TEXT("FireDamage") },
			{ TEXT("Ice"),      TEXT("IceDamage") },
			{ TEXT("Electric"), TEXT("ElectricDamage") },
			{ TEXT("Abyssal"),  TEXT("AbyssalDamage") },
		};

		FString Text;
		for (const auto& Channel : Channels)
		{
			const TSharedPtr<FJsonObject>* RangeObject;
			if (!DamageObject->TryGetObjectField(FString(FieldPrefix) + Channel[1], RangeObject))
			{
				continue;
			}

			int32 MinDamage = 0;
			int32 MaxDamage = 0;
			(*RangeObject)->TryGetNumberField(TEXT("x"), MinDamage);
			(*RangeObject)->TryGetNumberField(TEXT("y"), MaxDamage);
			if (MinDamage == 0 && MaxDamage == 0)
			{
				continue;
			}

			Text += FString::Printf(TEXT("  %s: %d-%d\n"), Channel[0], MinDamage, MaxDamage);
		}
		return Text;
	};

	FString WeaponDamageText;
	const TSharedPtr<FJsonObject>* DamageMapObject;
	const TSharedPtr<FJsonObject>* DamageObject;
	if (ItemObject->TryGetObjectField(TEXT("weaponLocalDamage"), DamageMapObject)
		&& (*DamageMapObject)->TryGetObjectField(TEXT("LocalDamage"), DamageObject))
	{
		WeaponDamageText = BuildDamageLinesFromJson(*DamageObject, TEXT("local"));
	}
	if (WeaponDamageText.IsEmpty()
		&& ItemObject->TryGetObjectField(TEXT("weaponDamage"), DamageMapObject)
		&& (*DamageMapObject)->TryGetObjectField(TEXT("BaseDamage"), DamageObject))
	{
		WeaponDamageText = BuildDamageLinesFromJson(*DamageObject, TEXT("base"));
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

		RemoveSingleImageButtonPadding(StashItemWidget);
		ItemSlotBox->AddChild(StashItemWidget);

		ULoadItemButtonProxy* Proxy = NewObject<ULoadItemButtonProxy>(this);
		Proxy->ItemUUID = SavedItem.Key;
		Proxy->OwningMenu = this;
		PlayerStashButtonProxies.Add(Proxy);
		InnerButton->OnClicked.AddDynamic(Proxy, &ULoadItemButtonProxy::HandleClicked);

		PlayerStashUniGrid->AddChildToUniformGrid(ItemSlotBox, Index / NumColumns, Index % NumColumns);

		++Index;
	}

	if (StashSelectButton)
	{
		StashSelectButton->SetVisibility(Index > 0 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UCoreMenu::PopulateCurrencyGrids()
{
	if (!InscriptionUniGrid || !GlyphUniGrid)
	{
		UE_LOG(LogTemp, Error, TEXT("InscriptionUniGrid or GlyphUniGrid is null or not found!"));
		return;
	}

	InscriptionUniGrid->ClearChildren();
	GlyphUniGrid->ClearChildren();
	InscriptionUniGrid->SetSlotPadding(FMargin(2.f));
	GlyphUniGrid->SetSlotPadding(FMargin(2.f));
	CurrencyButtonProxies.Empty();
	CurrencyCountTextBlocks.Empty();

	if (!CurrencyDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Currency_DT data table is not loaded."));
		return;
	}

	UClass* SingleImageButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleImageButton.WBP_SingleImageButton_C"));
	if (!SingleImageButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleImageButton class."));
		return;
	}

	int32 InscriptionIndex = 0;
	int32 GlyphIndex = 0;

	for (const FName& RowName : CurrencyDataTable->GetRowNames())
	{
		const FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CoreMenu::PopulateCurrencyGrids"));
		if (!CurrencyRow)
		{
			continue;
		}

		// The CurrencyName decides which grid this currency belongs in.
		UUniformGridPanel* TargetGrid = nullptr;
		int32* TargetIndex = nullptr;
		const FString CurrencyName = CurrencyRow->CurrencyName.ToString();
		if (CurrencyName.Contains(TEXT("Inscription"), ESearchCase::IgnoreCase))
		{
			TargetGrid = InscriptionUniGrid;
			TargetIndex = &InscriptionIndex;
		}
		else if (CurrencyName.Contains(TEXT("Glyph"), ESearchCase::IgnoreCase))
		{
			TargetGrid = GlyphUniGrid;
			TargetIndex = &GlyphIndex;
		}
		else
		{
			continue;
		}

		UUserWidget* CurrencyWidget = CreateWidget<UUserWidget>(this, SingleImageButtonClass);
		USizeBox* IconSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		UOverlay* IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!CurrencyWidget || !IconSlotBox || !IconOverlay || !CountText)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to construct WBP_SingleImageButton/SizeBox/Overlay/TextBlock for currency slot."));
			continue;
		}

		UButton* InnerButton = nullptr;
		if (FObjectProperty* ButtonProp = FindFProperty<FObjectProperty>(CurrencyWidget->GetClass(), TEXT("SingleImageButton")))
		{
			InnerButton = Cast<UButton>(ButtonProp->GetPropertyValue_InContainer(CurrencyWidget));
		}
		if (!InnerButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleImageButton property not found on WBP_SingleImageButton."));
			continue;
		}

		// Same reasoning as the shop/stash grids: forcing the cell size on a SizeBox keeps every
		// uniform grid cell exactly CurrencyIconSize regardless of the source texture's resolution.
		IconSlotBox->SetWidthOverride(CurrencyIconSize);
		IconSlotBox->SetHeightOverride(CurrencyIconSize);
		RemoveSingleImageButtonPadding(CurrencyWidget);
		IconSlotBox->AddChild(IconOverlay);

		// Overlay layers the stack count over the button's bottom-right corner. The text ignores hit
		// testing so clicks still reach the button underneath.
		if (UOverlaySlot* ButtonSlot = IconOverlay->AddChildToOverlay(CurrencyWidget))
		{
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}

		FSlateFontInfo CountFont = CountText->GetFont();
		CountFont.Size = CurrencyCountFontSize;
		CountText->SetFont(CountFont);
		CountText->SetShadowOffset(FVector2D(1.f, 1.f));
		CountText->SetShadowColorAndOpacity(FLinearColor::Black);
		CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* TextSlot = IconOverlay->AddChildToOverlay(CountText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Right);
			TextSlot->SetVerticalAlignment(VAlign_Bottom);
			TextSlot->SetPadding(FMargin(0.f, 0.f, 3.f, 1.f));
		}

		const FString CurrencyId = CurrencyRow->CurrencyId.ToString();
		CurrencyCountTextBlocks.Add(CurrencyId, CountText);
		UpdateCurrencyCountText(CurrencyId);

		UCurrencyButtonProxy* Proxy = NewObject<UCurrencyButtonProxy>(this);
		Proxy->RowName = RowName;
		Proxy->OwningMenu = this;
		CurrencyButtonProxies.Add(Proxy);
		InnerButton->OnClicked.AddDynamic(Proxy, &UCurrencyButtonProxy::HandleClicked);
		InnerButton->SetToolTip(CreateCurrencyToolTip(RowName));

		TargetGrid->AddChildToUniformGrid(IconSlotBox, *TargetIndex / CurrencyGridColumns, *TargetIndex % CurrencyGridColumns);
		++(*TargetIndex);

		LoadCurrencyIcon(GetSingleImageButtonIcon(CurrencyWidget), RowName);
	}
}

void UCurrencyButtonProxy::HandleClicked()
{
	if (!OwningMenu)
	{
		return;
	}

	if (bIsShopListing)
	{
		OwningMenu->OnShopCurrencyButtonClicked(RowName);
	}
	else
	{
		OwningMenu->OnCurrencyButtonClicked(RowName);
	}
}

void UCoreMenu::OnCurrencyButtonClicked(FName RowName)
{
	if (!CurrencyDataTable)
	{
		return;
	}

	const FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CoreMenu::OnCurrencyButtonClicked"));
	if (!CurrencyRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Currency_DT row found for RowName: %s"), *RowName.ToString());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Currency button clicked for CurrencyId: %s"), *CurrencyRow->CurrencyId.ToString());
	SetSelectedCurrencyId(CurrencyRow->CurrencyId.ToString());

	if (!ActiveItemRollButton)
	{
		UE_LOG(LogTemp, Error, TEXT("ActiveItemRollButton is null or not found!"));
		return;
	}
	LoadCurrencyIcon(GetSingleImageButtonIcon(ActiveItemRollButton), RowName);
	if (UButton* RollButton = GetSingleImageButton(ActiveItemRollButton))
	{
		RollButton->SetToolTip(CreateCurrencyToolTip(RowName));
	}
}

UWidget* UCoreMenu::CreateCurrencyToolTip(const FName& RowName)
{
	if (!CurrencyDataTable)
	{
		return nullptr;
	}

	const FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CoreMenu::CreateCurrencyToolTip"));
	if (!CurrencyRow)
	{
		return nullptr;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UTextBlock* DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!Background || !Lines || !NameText || !DescriptionText)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to construct currency tooltip widgets."));
		return nullptr;
	}

	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.92f));
	Background->SetPadding(FMargin(8.f));
	Background->SetContent(Lines);

	FSlateFontInfo NameFont = NameText->GetFont();
	NameFont.Size = CurrencyToolTipNameFontSize;
	NameFont.TypefaceFontName = TEXT("Bold");
	NameText->SetFont(NameFont);
	NameText->SetText(CurrencyRow->CurrencyName);
	Lines->AddChildToVerticalBox(NameText);

	FSlateFontInfo DescriptionFont = DescriptionText->GetFont();
	DescriptionFont.Size = CurrencyToolTipDescriptionFontSize;
	DescriptionText->SetFont(DescriptionFont);
	DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
	DescriptionText->SetText(CurrencyRow->CurrencyDescription);
	// A tooltip has no width constraint of its own, so wrap at a fixed width instead of auto-wrapping.
	DescriptionText->SetWrapTextAt(CurrencyToolTipWrapWidth);
	if (UVerticalBoxSlot* DescriptionSlot = Lines->AddChildToVerticalBox(DescriptionText))
	{
		DescriptionSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	return Background;
}

void UCoreMenu::LoadCurrencyIcon(UImage* IconImage, const FName& RowName)
{
	if (!IconImage || !CurrencyDataTable)
	{
		return;
	}

	const FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CoreMenu::LoadCurrencyIcon"));
	if (!CurrencyRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Currency_DT row found for RowName: %s"), *RowName.ToString());
		return;
	}

	if (!CurrencyRow->CurrencyAssetData.CurrencyIcon)
	{
		UE_LOG(LogTemp, Warning, TEXT("No CurrencyIcon set for currency: %s"), *CurrencyRow->CurrencyId.ToString());
		return;
	}

	IconImage->SetBrushFromTexture(CurrencyRow->CurrencyAssetData.CurrencyIcon);
	IconImage->SetDesiredSizeOverride(FVector2D(CurrencyIconSize, CurrencyIconSize));
	// Not hit-testable so clicks still reach the owning button.
	IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCoreMenu::SetCurrencyStackCounts(const TMap<FString, int32>& StackCounts)
{
	CurrencyStackCounts = StackCounts;

	for (const TPair<FString, TObjectPtr<UTextBlock>>& Entry : CurrencyCountTextBlocks)
	{
		UpdateCurrencyCountText(Entry.Key);
	}
}

void UCoreMenu::UpdateCurrencyCountText(const FString& CurrencyId)
{
	const TObjectPtr<UTextBlock>* CountText = CurrencyCountTextBlocks.Find(CurrencyId);
	if (!CountText || !*CountText)
	{
		return;
	}

	const int32* Count = CurrencyStackCounts.Find(CurrencyId);
	(*CountText)->SetText(FText::AsNumber(Count ? *Count : 0));
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

	if (!bShouldShow)
	{
		// Closing the shop drops any currency listing selection, so a later Buy doesn't purchase it unseen.
		SelectedShopCurrencyId.Empty();
	}

	if (PlayerStashHorizBox)
	{
		SetPanelAndChildrenVisibility(PlayerStashHorizBox, ESlateVisibility::Hidden);
	}

	RefreshActiveItemDisplay();

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

	UClass* SingleImageButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleImageButton.WBP_SingleImageButton_C"));
	if (!SingleImageButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleImageButton class."));
		return;
	}

	constexpr int32 NumColumns = 4;
	constexpr float ShopItemSlotSize = 128.f;
	constexpr int32 ShopCurrencyNumColumns = 8;
	constexpr float ShopCurrencySlotSize = 64.f;
	int32 Index = 0;

	// A missing BaseItem_DT only skips the item listings; the basic currency row below still populates.
	const TArray<FName> NoShopItems;
	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseItem_DT data table is not loaded."));
	}
	const TArray<FName>& ShopItemRowNames = ItemDataTable ? CurrentShopItems : NoShopItems;

	for (const FName& RowName : ShopItemRowNames)
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

		RemoveSingleImageButtonPadding(ShopItemWidget);
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

	// Basic currencies are rebuilt from Currency_DT on every repopulation (not from CurrentShopItems),
	// so buying one never removes it.
	PopulateShopCurrencyGrid(SingleImageButtonClass, ShopCurrencyNumColumns, ShopCurrencySlotSize);
}

void UCoreMenu::PopulateShopCurrencyGrid(UClass* SingleImageButtonClass, int32 NumColumns, float SlotSize)
{
	if (!ShopCurrencyUniGrid)
	{
		UE_LOG(LogTemp, Error, TEXT("ShopCurrencyUniGrid is null or not found!"));
		return;
	}

	ShopCurrencyUniGrid->ClearChildren();
	ShopCurrencyUniGrid->SetSlotPadding(FMargin(4.f));
	ShopCurrencyButtonProxies.Empty();

	if (!CurrencyDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Currency_DT data table is not loaded."));
		return;
	}

	int32 Index = 0;

	for (const FName& RowName : CurrencyDataTable->GetRowNames())
	{
		const FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CoreMenu::PopulateShopCurrencyGrid"));
		if (!CurrencyRow || CurrencyRow->BaseCurrencyType != EBaseCurrencyType::Basic)
		{
			continue;
		}

		UUserWidget* CurrencyWidget = CreateWidget<UUserWidget>(this, SingleImageButtonClass);
		USizeBox* CurrencySlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (!CurrencyWidget || !CurrencySlotBox)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to construct WBP_SingleImageButton/SizeBox for shop currency."));
			continue;
		}

		UButton* InnerButton = GetSingleImageButton(CurrencyWidget);
		if (!InnerButton)
		{
			continue;
		}

		// Same reasoning as the other grids: forcing the cell size on a SizeBox keeps every uniform grid
		// cell exactly SlotSize regardless of the source texture's resolution.
		CurrencySlotBox->SetWidthOverride(SlotSize);
		CurrencySlotBox->SetHeightOverride(SlotSize);
		RemoveSingleImageButtonPadding(CurrencyWidget);
		CurrencySlotBox->AddChild(CurrencyWidget);

		if (UImage* Icon = GetSingleImageButtonIcon(CurrencyWidget))
		{
			LoadCurrencyIcon(Icon, RowName);
			Icon->SetDesiredSizeOverride(FVector2D(SlotSize, SlotSize));
		}

		UCurrencyButtonProxy* Proxy = NewObject<UCurrencyButtonProxy>(this);
		Proxy->RowName = RowName;
		Proxy->bIsShopListing = true;
		Proxy->OwningMenu = this;
		ShopCurrencyButtonProxies.Add(Proxy);
		InnerButton->OnClicked.AddDynamic(Proxy, &UCurrencyButtonProxy::HandleClicked);
		InnerButton->SetToolTip(CreateCurrencyToolTip(RowName));

		ShopCurrencyUniGrid->AddChildToUniformGrid(CurrencySlotBox, Index / NumColumns, Index % NumColumns);

		++Index;
	}
}

void UCoreMenu::OnShopCurrencyButtonClicked(FName RowName)
{
	if (!CurrencyDataTable)
	{
		return;
	}

	const FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CoreMenu::OnShopCurrencyButtonClicked"));
	if (!CurrencyRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Currency_DT row found for RowName: %s"), *RowName.ToString());
		return;
	}

	SelectedShopCurrencyId = CurrencyRow->CurrencyId.ToString();
	UE_LOG(LogTemp, Warning, TEXT("Shop currency button clicked for CurrencyId: %s"), *SelectedShopCurrencyId);

	SetActiveItemText(FString::Printf(
		TEXT("%s\n%s\nCost: %d gold"),
		*CurrencyRow->CurrencyName.ToString(),
		*CurrencyRow->CurrencyDescription.ToString(),
		BasicCurrencyShopCost));
}

void UCoreMenu::RefreshShopGrid()
{
	PopulateShopGrid();
}

void UCoreMenu::RemoveItemFromShop(const FString& ItemId)
{
	// Called by ItemHandler once a purchase has actually succeeded, so a rejected (unaffordable)
	// buy leaves the listing in CurrentShopItems instead of removing it for nothing.
	if (!ItemDataTable)
	{
		return;
	}

	// Matches on ItemId rather than array index so a duplicate listing elsewhere in the shop isn't
	// accidentally removed instead.
	for (int32 Index = 0; Index < CurrentShopItems.Num(); ++Index)
	{
		const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(CurrentShopItems[Index], TEXT("CoreMenu::RemoveItemFromShop"));
		if (ItemRow && ItemRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
		{
			CurrentShopItems.RemoveAt(Index);
			break;
		}
	}

	RefreshShopGrid();
}

void UCoreMenu::OnShopItemButtonClicked(FString ItemId, FString ItemUUID)
{
	UE_LOG(LogTemp, Warning, TEXT("Shop item button clicked for ItemId: %s (UUID: %s)"), *ItemId, *ItemUUID);
	SelectItemData(FText::FromString(ItemId));
	SelectedItemUUID = ItemUUID;
	SelectedShopCurrencyId.Empty();

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
	RefreshActiveItemDisplay();
}

void UCoreMenu::OnResetGameButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("ResetGameButton Clicked."));

	// Listeners (ItemHandler, PlayerInventory) delete/clear their own save files
	// (SavedItems.json, SavedCurrency.json) synchronously in response to this broadcast.
	OnResetGameButtonClickedEvent.Broadcast();

	// The previously active/selected item may no longer exist after the reset.
	SelectedItemData = FBaseItemStruct();
	SelectedItemId.Empty();
	SelectedItemUUID.Empty();
	bHasSelectedItemData = false;
	SetActiveItemText(TEXT(""));
	UpdatePanelVisibility({ ActiveItemImageHorizBox }, ESlateVisibility::Hidden);

	PopulatePlayerStash();
	RandomizeShopItems();
}

void UCoreMenu::UpdatePanelVisibility(const TArray<UPanelWidget*>& Panels, ESlateVisibility NewVisibility)
{
	for (UPanelWidget* Panel : Panels)
	{
		SetPanelAndChildrenVisibility(Panel, NewVisibility);
	}
}

void UCoreMenu::RefreshActiveItemDisplay()
{
	const bool bShopOpen = ShopWindowBox && ShopWindowBox->GetVisibility() == ESlateVisibility::Visible;
	const bool bStashOpen = PlayerStashHorizBox && PlayerStashHorizBox->GetVisibility() == ESlateVisibility::Visible;

	if (!bShopOpen && !bStashOpen && bHasSelectedItemData)
	{
		ShowActiveItemImage();
	}
	else
	{
		UpdatePanelVisibility({ ActiveItemImageHorizBox }, ESlateVisibility::Hidden);
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

			// Recurse into nested panels (e.g. BuyBox inside ShopWindowBox) - an ancestor left at its
			// design-time visibility would otherwise block rendering/hit-testing for everything below
			// it even though this panel and its immediate children report Visible.
			if (UPanelWidget* ChildPanel = Cast<UPanelWidget>(Child))
			{
				SetPanelAndChildrenVisibility(ChildPanel, NewVisibility);
			}
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

	if (ItemDataTableRowCount <= 0 || ItemDataTableRowNames.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("RandomizeShopItems: BaseItem_DT has no rows loaded; skipping."));
		return;
	}

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

void UCoreMenu::SetWarningText(const FString& NewMessage)
{
	if (!WarningsTextBox)
	{
		UE_LOG(LogTemp, Error, TEXT("WarningsTextBox is null or not found!"));
		return;
	}

	WarningsTextBox->SetText(FText::FromString(NewMessage));

	GetWorld()->GetTimerManager().ClearTimer(WarningTextTimerHandle);
	if (!NewMessage.IsEmpty())
	{
		GetWorld()->GetTimerManager().SetTimer(WarningTextTimerHandle, this, &UCoreMenu::ClearWarningText, 3.f, false);
	}
}

void UCoreMenu::ClearWarningText()
{
	if (WarningsTextBox)
	{
		WarningsTextBox->SetText(FText::GetEmpty());
	}
}


