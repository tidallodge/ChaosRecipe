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
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "BaseItemStruct.h"
#include "CurrencyStruct.h"
#include "ItemInstanceManager.h"
#include "MinionInstanceManager.h"
#include "MinionClassStruct.h"
#include "ZoneStruct.h"
#include "DataTableHelpers.h"
#include "PlayerMinion.h"
#include "Enemy.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Algo/RandomShuffle.h"

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

// Resolves the MinionButton button inside a WBP_MinionButton instance (null if missing).
static UButton* GetMinionButton(UUserWidget* MinionButtonWidget)
{
	if (!MinionButtonWidget)
	{
		return nullptr;
	}

	UButton* Button = Cast<UButton>(MinionButtonWidget->GetWidgetFromName(TEXT("MinionButton")));
	if (!Button)
	{
		UE_LOG(LogTemp, Warning, TEXT("MinionButton not found on WBP_MinionButton."));
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

void UCoreMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Done here rather than in NativeConstruct because it changes the layout, which only shows up if it
	// happens before the Slate widgets are built.
	InitDamageNumberStack(BattleMinionHPText, MinionDamageNumbers);
	InitDamageNumberStack(BattleEnemyHPText, EnemyDamageNumbers);
	EnemyHPBarsBox = SwapForVerticalBox(BattleEnemyHPBar);

	RegisterWindows();
	PopulateMap();
	// Here too, since the hover flyouts it binds are only picked up when the Slate widgets are built.
	InitMinionEquipButtons();
}

void UCoreMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickDamageNumbers(MinionDamageNumbers, InDeltaTime);
	TickDamageNumbers(EnemyDamageNumbers, InDeltaTime);
	TickWarningText(InDeltaTime);
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
    ValidateButton(StashCraftButton);
    ValidateButton(StashAssignButton);
    ValidateButton(ResetGameButton);
    ValidateButton(BattlePlayButton);
    ValidateButton(HireShopButton);
    ValidateButton(HireShopRerollButton);
    ValidateButton(HireShopHireButton);
    ValidateButton(MapButton);
    ValidateButton(ReserveMinionButton);

	BuyButton->OnClicked.AddDynamic(this, &UCoreMenu::OnBuyButtonClicked);
	SellButton->OnClicked.AddDynamic(this, &UCoreMenu::OnSellButtonClicked);
	ShopButton->OnClicked.AddDynamic(this, &UCoreMenu::OnShopButtonClicked);
	RandomizeShopButton->OnClicked.AddDynamic(this, &UCoreMenu::OnRandomizeShopButtonClicked);
	PlayerStashButton->OnClicked.AddDynamic(this, &UCoreMenu::OnPlayerStashButtonClicked);
	StashCraftButton->OnClicked.AddDynamic(this, &UCoreMenu::OnStashCraftButtonClicked);
	StashAssignButton->OnClicked.AddDynamic(this, &UCoreMenu::OnStashAssignButtonClicked);
	ResetGameButton->OnClicked.AddDynamic(this, &UCoreMenu::OnResetGameButtonClicked);
	BattlePlayButton->OnClicked.AddDynamic(this, &UCoreMenu::OnBattlePlayButtonClicked);
	HireShopButton->OnClicked.AddDynamic(this, &UCoreMenu::OnHireShopButtonClicked);
	HireShopRerollButton->OnClicked.AddDynamic(this, &UCoreMenu::OnHireShopRerollButtonClicked);
	HireShopHireButton->OnClicked.AddDynamic(this, &UCoreMenu::OnHireShopHireButtonClicked);
	ReserveMinionButton->OnClicked.AddDynamic(this, &UCoreMenu::OnReserveMinionButtonClicked);
	MapButton->OnClicked.AddDynamic(this, &UCoreMenu::OnMapButtonClicked);

	HideWindow(ECoreMenuWindow::ShopWindow);
	HideWindow(ECoreMenuWindow::MinionStash);
	HideWindow(ECoreMenuWindow::Map);

	// Starts out matching however ActiveItemImageHorizBox is set up in WBP_CoreMenu.
	if (IsWindowVisible(ECoreMenuWindow::ActiveItemImage))
	{
		ShowWindow(ECoreMenuWindow::ActiveItemText);
	}
	else
	{
		HideWindow(ECoreMenuWindow::ActiveItemText);
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
	// Clicking ActiveItemRollButton is handled by ItemHandler, which binds to it via GetActiveItemRollButton.

	// Off by default, so the fill colors set in WBP_CoreMenu are kept.
	if (bOverrideHPBarColors)
	{
		BattleMinionHPBar->SetFillColorAndOpacity(MinionHPBarColor);
		BattleEnemyHPBar->SetFillColorAndOpacity(EnemyHPBarColor);
	}
	MinionHPBarDefaultColor = BattleMinionHPBar->GetFillColorAndOpacity();
	EnemyHPBarDefaultColor = BattleEnemyHPBar->GetFillColorAndOpacity();

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

	ClearSelectedItem();
	HideActiveItem();
	ShowWindow(ECoreMenuWindow::PlayerStash);

	// The broadcast above already removed the sold item from SavedItems.json (synchronously, via
	// ItemHandler::OnSellButtonClicked), so repopulate now the stash grid no longer shows it.
	PopulatePlayerStash();
}

void UCoreMenu::OnBuyButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("BuyButton Clicked. Event Dispatched"));
	// Unlike Sell, Buy keeps the ShopWindow window open so the player can keep shopping, and leaves the
	// ActiveItemImage window alone rather than showing the purchased item there.
	if (!bHasSelectedItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("No selected item to buy."));
		return;
	}
	// ItemHandler::OnBuyButtonClicked runs synchronously here and rejects the purchase (no stash
	// save, no shop removal) if the player can't afford it, so CoreMenu doesn't remove the listing
	// itself - it only knows the purchase succeeded once ItemHandler calls RemoveItemFromShop back.
	OnBuyButtonClickedEvent.Broadcast(SelectedItemId, SelectedItemUUID);

	// A successful purchase has already been saved to SavedItems.json during the broadcast, so the stash is
	// rebuilt to show it (a rejected one leaves the stash as it was).
	PopulatePlayerStash();
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

UButton* UCoreMenu::GetActiveItemRollButton() const
{
	return GetSingleImageButton(ActiveItemRollButton);
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

	SetSelectedItemUUID(ItemUUID);

	FString ItemId;
	ItemObject->TryGetStringField(TEXT("ItemId"), ItemId);

	// Marks this stash item as the active item.
	SelectItemData(FText::FromString(ItemId));

	// Lets listeners (e.g. ItemHandler) load this existing saved item's stats into their
	// working cache, keyed to its already-assigned UUID, so Randomize/Save edit it in place
	// instead of a stale or nonexistent cached item. ItemHandler also shows it in ActiveItemTextBox,
	// laid out for its class (weapon, armor or jewelry).
	OnStashItemSelectedEvent.Broadcast(ItemId, ItemUUID);
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
	StashItemSelectionBorders.Empty();

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
	constexpr float StashItemSlotSize = 160.f;
	int32 Index = 0;

	UItemInstanceManager StashManager;
	// Read-only, like StashManager: MinionHandler's own instance is the only one that writes SavedMinions.json.
	const UMinionInstanceManager SavedMinionsManager;

	for (const TPair<FString, TSharedPtr<FJsonObject>>& SavedItem : StashManager.GetSavedItems())
	{
		// An item assigned to a minion stays in SavedItems.json, but isn't in the stash.
		if (!SavedMinionsManager.FindMinionForItem(SavedItem.Key).IsEmpty())
		{
			continue;
		}

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
		ItemSlotBox->AddChild(WrapInSelectionBorder(StashItemWidget, SavedItem.Key, SelectedItemUUID, StashItemSelectionBorders));

		ULoadItemButtonProxy* Proxy = NewObject<ULoadItemButtonProxy>(this);
		Proxy->ItemUUID = SavedItem.Key;
		Proxy->OwningMenu = this;
		PlayerStashButtonProxies.Add(Proxy);
		InnerButton->OnClicked.AddDynamic(Proxy, &ULoadItemButtonProxy::HandleClicked);
		// Has to be bound before the grid builds the button's Slate widget, which is when it's picked up.
		InnerButton->ToolTipWidgetDelegate.BindDynamic(Proxy, &ULoadItemButtonProxy::CreateToolTip);

		PlayerStashUniGrid->AddChildToUniformGrid(ItemSlotBox, Index / NumColumns, Index % NumColumns);

		++Index;
	}

	// Nothing to craft or assign with an empty stash.
	for (UButton* StashButton : { StashCraftButton, StashAssignButton })
	{
		if (StashButton)
		{
			StashButton->SetVisibility(Index > 0 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
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
	if (OwningMenu)
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

	return CreateTitledToolTip(CurrencyRow->CurrencyName, CurrencyRow->CurrencyDescription);
}

UWidget* ULoadItemButtonProxy::CreateToolTip()
{
	// A saved item's ItemId comes from its save, so its UUID is enough.
	return OwningMenu ? OwningMenu->CreateItemToolTip(FString(), ItemUUID) : nullptr;
}

UWidget* UShopItemButtonProxy::CreateToolTip()
{
	return OwningMenu ? OwningMenu->CreateItemToolTip(ItemId, ItemUUID) : nullptr;
}

UWidget* UCoreMenu::CreateItemToolTip(const FString& ItemId, const FString& ItemUUID)
{
	const FString DisplayText = GetItemDisplayTextDelegate.IsBound() ? GetItemDisplayTextDelegate.Execute(ItemId, ItemUUID) : FString();
	if (DisplayText.IsEmpty())
	{
		return nullptr;
	}

	// The display text's first line is the item's name, which becomes the flyout's title.
	FString ItemName;
	FString ItemStats;
	if (!DisplayText.Split(TEXT("\n"), &ItemName, &ItemStats))
	{
		ItemName = DisplayText;
	}

	ItemToolTip = CreateTitledToolTip(FText::FromString(ItemName), FText::FromString(ItemStats.TrimEnd()));
	return ItemToolTip;
}

UWidget* UCoreMenu::CreateTitledToolTip(const FText& Title, const FText& Body)
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!Background || !Lines || !TitleText || !BodyText)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to construct tooltip widgets."));
		return nullptr;
	}

	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.92f));
	Background->SetPadding(FMargin(8.f));
	Background->SetContent(Lines);

	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = ToolTipTitleFontSize;
	TitleFont.TypefaceFontName = TEXT("Bold");
	TitleText->SetFont(TitleFont);
	TitleText->SetText(Title);
	Lines->AddChildToVerticalBox(TitleText);

	FSlateFontInfo BodyFont = BodyText->GetFont();
	BodyFont.Size = ToolTipBodyFontSize;
	BodyText->SetFont(BodyFont);
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
	BodyText->SetText(Body);
	// A tooltip has no width constraint of its own, so wrap at a fixed width instead of auto-wrapping.
	BodyText->SetWrapTextAt(ToolTipWrapWidth);
	if (UVerticalBoxSlot* BodySlot = Lines->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
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

void UCoreMenu::SetMinionButtons(const TArray<UPlayerMinion*>& Minions)
{
	PopulateMinionStash(Minions);

	if (!MinionButtonsVertBox)
	{
		UE_LOG(LogTemp, Error, TEXT("MinionButtonsVertBox is null or not found!"));
		return;
	}

	MinionButtonsVertBox->ClearChildren();
	MinionButtonProxies.Empty();

	UClass* MinionButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_MinionButton.WBP_MinionButton_C"));
	if (!MinionButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_MinionButton class."));
		return;
	}

	for (UPlayerMinion* Minion : Minions)
	{
		if (MinionButtonsVertBox->GetChildrenCount() >= MaxMinionButtons)
		{
			break;
		}
		// The rest are in the minion stash.
		if (!Minion || !Minion->IsActivePartyMember())
		{
			continue;
		}

		UUserWidget* MinionButtonWidget = CreateMinionButtonWidget(MinionButtonClass, Minion->GetMinionIcon(), Minion->GetMinionClassName());
		UButton* MinionButton = GetMinionButton(MinionButtonWidget);
		if (!MinionButton)
		{
			continue;
		}

		UMinionButtonProxy* Proxy = NewObject<UMinionButtonProxy>(this);
		Proxy->Minion = Minion;
		Proxy->OwningMenu = this;
		MinionButtonProxies.Add(Proxy);
		MinionButton->OnClicked.AddDynamic(Proxy, &UMinionButtonProxy::HandleClicked);

		MinionButtonsVertBox->AddChildToVerticalBox(MinionButtonWidget);
	}
}

void UCoreMenu::PopulateMinionStash(const TArray<UPlayerMinion*>& Minions)
{
	if (!MinionStashButtonsVertBox)
	{
		UE_LOG(LogTemp, Error, TEXT("MinionStashButtonsVertBox is null or not found!"));
		return;
	}

	MinionStashButtonsVertBox->ClearChildren();
	MinionStashButtonProxies.Empty();
	MinionStashSelectionBorders.Empty();

	UClass* MinionButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_MinionButton.WBP_MinionButton_C"));
	if (!MinionButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_MinionButton class."));
		return;
	}

	for (UPlayerMinion* Minion : Minions)
	{
		// The active party is in MinionButtonsVertBox instead.
		if (!Minion || Minion->IsActivePartyMember())
		{
			continue;
		}

		UUserWidget* MinionButtonWidget = CreateMinionButtonWidget(MinionButtonClass, Minion->GetMinionIcon(), Minion->GetMinionClassName());
		UButton* MinionButton = GetMinionButton(MinionButtonWidget);
		if (!MinionButton)
		{
			continue;
		}

		UMinionStashButtonProxy* Proxy = NewObject<UMinionStashButtonProxy>(this);
		Proxy->Minion = Minion;
		Proxy->OwningMenu = this;
		MinionStashButtonProxies.Add(Proxy);
		MinionButton->OnClicked.AddDynamic(Proxy, &UMinionStashButtonProxy::HandleClicked);

		MinionStashButtonsVertBox->AddChild(WrapInSelectionBorder(MinionButtonWidget, Minion->GetMinionUUID(), SelectedMinionUUID, MinionStashSelectionBorders));
	}
}

void UCoreMenu::OnReserveMinionButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("ReserveMinionButton Clicked."));

	if (IsWindowVisible(ECoreMenuWindow::MinionStash))
	{
		HideWindow(ECoreMenuWindow::MinionStash);
		RefreshActiveItemDisplay();
		return;
	}

	ShowWindow(ECoreMenuWindow::MinionStash);
}

UUserWidget* UCoreMenu::CreateMinionButtonWidget(UClass* MinionButtonClass, UTexture2D* MinionIcon, const FText& MinionName)
{
	UUserWidget* MinionButtonWidget = CreateWidget<UUserWidget>(this, MinionButtonClass);
	if (!MinionButtonWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to construct WBP_MinionButton for %s."), *MinionName.ToString());
		return nullptr;
	}

	// Found by name rather than as Blueprint variables, since UMG only marks Buttons "Is Variable" by
	// default, not Images or Text Blocks.
	if (UImage* MinionButtonIcon = Cast<UImage>(MinionButtonWidget->GetWidgetFromName(TEXT("MinionButtonIcon"))))
	{
		if (MinionIcon)
		{
			MinionButtonIcon->SetBrushFromTexture(MinionIcon);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No MinionIcon set in MinionClass_DT for %s."), *MinionName.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MinionButtonIcon not found on WBP_MinionButton."));
	}

	if (UTextBlock* MinionButtonText = Cast<UTextBlock>(MinionButtonWidget->GetWidgetFromName(TEXT("MinionButtonText"))))
	{
		MinionButtonText->SetText(MinionName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MinionButtonText not found on WBP_MinionButton."));
	}

	return MinionButtonWidget;
}

void UCoreMenu::OnHireShopButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("HireShopButton Clicked."));
	ShowWindow(ECoreMenuWindow::HireShop);
}

void UCoreMenu::OnHireShopRerollButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("HireShopRerollButton Clicked."));
	OnHireShopRerollButtonClickedEvent.Broadcast();
}

void UCoreMenu::OnHireShopHireButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("HireShopHireButton Clicked."));
	if (SelectedHireShopMinionUUID.IsEmpty())
	{
		SetWarningText(TEXT("Select a minion in the hire shop first."));
		return;
	}

	OnHireShopHireButtonClickedEvent.Broadcast(SelectedHireShopMinionUUID);
}

void UCoreMenu::SetHireShopMinions(const TArray<EMinionClass>& MinionClasses, const TArray<FString>& MinionUUIDs)
{
	CurrentHireShopMinions = MinionClasses;
	CurrentHireShopMinionUUIDs = MinionUUIDs;
	if (!CurrentHireShopMinionUUIDs.Contains(SelectedHireShopMinionUUID))
	{
		SelectedHireShopMinionUUID.Empty();
	}
	PopulateHireShop();
}

void UCoreMenu::PopulateHireShop()
{
	if (!HireShopButtonsVertBox)
	{
		UE_LOG(LogTemp, Error, TEXT("HireShopButtonsVertBox is null or not found!"));
		return;
	}

	HireShopButtonsVertBox->ClearChildren();
	HireShopMinionButtonProxies.Empty();
	HireShopMinionSelectionBorders.Empty();

	UClass* MinionButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_MinionButton.WBP_MinionButton_C"));
	if (!MinionButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_MinionButton class."));
		return;
	}

	for (int32 ListingIndex = 0; ListingIndex < CurrentHireShopMinions.Num(); ++ListingIndex)
	{
		if (!CurrentHireShopMinionUUIDs.IsValidIndex(ListingIndex))
		{
			continue;
		}
		const EMinionClass MinionClass = CurrentHireShopMinions[ListingIndex];
		const FString& ListingUUID = CurrentHireShopMinionUUIDs[ListingIndex];

		FMinionClassStruct ClassRow;
		const bool bFoundClass = DataTableHelpers::FindRow(MinionClassDataTablePath,
			[MinionClass](const FMinionClassStruct& Row) { return Row.MinionClass == MinionClass; }, ClassRow);
		if (!bFoundClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("No MinionClass_DT row for %s."), *UEnum::GetValueAsString(MinionClass));
			continue;
		}

		UUserWidget* MinionButtonWidget = CreateMinionButtonWidget(MinionButtonClass, ClassRow.MinionAssetData.MinionIcon, ClassRow.ClassName);
		UButton* MinionButton = GetMinionButton(MinionButtonWidget);
		if (!MinionButton)
		{
			continue;
		}

		UHireShopMinionButtonProxy* Proxy = NewObject<UHireShopMinionButtonProxy>(this);
		Proxy->MinionUUID = ListingUUID;
		Proxy->MinionName = ClassRow.ClassName;
		Proxy->OwningMenu = this;
		HireShopMinionButtonProxies.Add(Proxy);
		MinionButton->OnClicked.AddDynamic(Proxy, &UHireShopMinionButtonProxy::HandleClicked);

		HireShopButtonsVertBox->AddChild(WrapInSelectionBorder(MinionButtonWidget, ListingUUID, SelectedHireShopMinionUUID, HireShopMinionSelectionBorders));
	}
}

void UHireShopMinionButtonProxy::HandleClicked()
{
	if (OwningMenu)
	{
		OwningMenu->OnHireShopMinionButtonClicked(MinionUUID, MinionName);
	}
}

void UCoreMenu::OnHireShopMinionButtonClicked(FString MinionUUID, FText MinionName)
{
	SelectedHireShopMinionUUID = MinionUUID;
	RefreshSelectionBorders(HireShopMinionSelectionBorders, SelectedHireShopMinionUUID);

	const FString Message = FString::Printf(TEXT("Selected Hire Shop Minion %s %s"), *MinionName.ToString(), *MinionUUID);
	LogToScreen(Message);
	SetWarningText(Message, 3.f);
}

void UCoreMenu::OnMapButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("MapButton Clicked."));

	if (IsWindowVisible(ECoreMenuWindow::Map))
	{
		HideWindow(ECoreMenuWindow::Map);
		RefreshActiveItemDisplay();
		return;
	}

	ShowWindow(ECoreMenuWindow::Map);
}

void UCoreMenu::PopulateMap()
{
	if (!MapVertBox)
	{
		UE_LOG(LogTemp, Error, TEXT("MapVertBox is null or not found!"));
		return;
	}

	UClass* SingleButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleButton.WBP_SingleButton_C"));
	if (!SingleButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleButton class."));
		return;
	}

	TArray<FZoneStruct> Zones;
	if (!DataTableHelpers::LoadAllRows(ZoneDataTablePath, Zones))
	{
		UE_LOG(LogTemp, Error, TEXT("Zone_DT has no zones to show on the map."));
		return;
	}

	for (const FZoneStruct& Zone : Zones)
	{
		UUserWidget* ZoneButtonWidget = CreateWidget<UUserWidget>(this, SingleButtonClass);
		UButton* ZoneButton = ZoneButtonWidget ? Cast<UButton>(ZoneButtonWidget->GetWidgetFromName(TEXT("SingleButton"))) : nullptr;
		if (!ZoneButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleButton not found on WBP_SingleButton."));
			continue;
		}

		// Found by name rather than as a Blueprint variable, since UMG only marks Buttons "Is Variable" by default.
		if (UTextBlock* ZoneButtonText = Cast<UTextBlock>(ZoneButtonWidget->GetWidgetFromName(TEXT("SingleButtonText"))))
		{
			ZoneButtonText->SetText(Zone.ZoneName.IsEmpty() ? Zone.ZoneId : Zone.ZoneName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SingleButtonText not found on WBP_SingleButton."));
		}

		UZoneButtonProxy* Proxy = NewObject<UZoneButtonProxy>(this);
		Proxy->ZoneId = Zone.ZoneId.ToString();
		Proxy->ZoneName = Zone.ZoneName;
		Proxy->OwningMenu = this;
		ZoneButtonProxies.Add(Proxy);
		ZoneButton->OnClicked.AddDynamic(Proxy, &UZoneButtonProxy::HandleClicked);

		MapVertBox->AddChild(ZoneButtonWidget);
	}
}

void UZoneButtonProxy::HandleClicked()
{
	if (OwningMenu)
	{
		OwningMenu->OnZoneButtonClicked(ZoneId, ZoneName);
	}
}

void UCoreMenu::OnZoneButtonClicked(FString ZoneId, FText ZoneName)
{
	ActiveZoneId = ZoneId;

	const FString Message = FString::Printf(TEXT("Zone changed to %s (%s)"), *ZoneName.ToString(), *ZoneId);
	LogToScreen(Message);
	SetWarningText(Message);
}

void UMinionButtonProxy::HandleClicked()
{
	if (OwningMenu)
	{
		OwningMenu->OnMinionButtonClicked(Minion);
	}
}

void UCoreMenu::OnMinionButtonClicked(UPlayerMinion* Minion)
{
	if (!Minion)
	{
		return;
	}

	SelectMinion(Minion, 6.f);
	ShowWindow(ECoreMenuWindow::MinionEquip);
}

void UMinionStashButtonProxy::HandleClicked()
{
	if (OwningMenu)
	{
		OwningMenu->OnMinionStashButtonClicked(Minion);
	}
}

void UCoreMenu::OnMinionStashButtonClicked(UPlayerMinion* Minion)
{
	if (Minion)
	{
		SelectMinion(Minion, 3.f);
	}
}

void UCoreMenu::SelectMinion(UPlayerMinion* Minion, float WarningDisplaySeconds)
{
	SelectedMinionUUID = Minion->GetMinionUUID();
	RefreshSelectionBorders(MinionStashSelectionBorders, SelectedMinionUUID);

	const FString Message = FString::Printf(TEXT("Selected Minion %s %s"), *Minion->GetMinionClassName().ToString(), *SelectedMinionUUID);
	LogToScreen(Message);
	SetWarningText(Message, WarningDisplaySeconds);

	// Unbound first even when the same minion is selected again, since a minion can't be bound twice.
	if (EquipWindowMinion)
	{
		EquipWindowMinion->OnSaveDataChangedEvent.RemoveDynamic(this, &UCoreMenu::OnEquipWindowMinionSaveDataChanged);
	}
	EquipWindowMinion = Minion;
	EquipWindowMinion->OnSaveDataChangedEvent.AddDynamic(this, &UCoreMenu::OnEquipWindowMinionSaveDataChanged);
	RefreshMinionEquipButtons();
}

void UCoreMenu::OnEquipWindowMinionSaveDataChanged(FString MinionUUID)
{
	RefreshMinionEquipButtons();
}

// The item slots a MinionEquip window slot button shows, from its name: Minion{ItemSlot}EquipButton, plus a _N suffix
// where more than one button shows a slot (e.g. MinionRingEquipButton_1 and _2). {ItemSlot} is an EItemSlot name, or
// one of the names WBP_CoreMenu uses for a slot instead. Empty if the name doesn't name a slot.
static TArray<EItemSlot> GetMinionEquipButtonItemSlots(const FString& ButtonName)
{
	// WBP_CoreMenu's names for the slots it calls something else. The main hand button shows two-handed weapons too.
	static const TMap<FString, TArray<EItemSlot>> SlotNameAliases = {
		{ TEXT("Helmet"),   { EItemSlot::Head } },
		{ TEXT("Amulet"),   { EItemSlot::Neck } },
		{ TEXT("Greaves"),  { EItemSlot::Boots } },
		{ TEXT("MainHand"), { EItemSlot::HandMain, EItemSlot::HandBoth } },
		{ TEXT("OffHand"),  { EItemSlot::HandOff } },
	};

	const int32 SuffixIndex = ButtonName.Find(TEXT("EquipButton"), ESearchCase::CaseSensitive);
	if (SuffixIndex == INDEX_NONE)
	{
		return {};
	}

	FString SlotName = ButtonName.Left(SuffixIndex);
	if (!SlotName.RemoveFromStart(TEXT("Minion"), ESearchCase::CaseSensitive))
	{
		return {};
	}

	if (const TArray<EItemSlot>* AliasedSlots = SlotNameAliases.Find(SlotName))
	{
		return *AliasedSlots;
	}

	const int64 SlotValue = StaticEnum<EItemSlot>()->GetValueByNameString(SlotName);
	if (SlotValue == INDEX_NONE)
	{
		return {};
	}
	return { static_cast<EItemSlot>(SlotValue) };
}

void UCoreMenu::InitMinionEquipButtons()
{
	if (!MinionEqiupCanvas)
	{
		UE_LOG(LogTemp, Error, TEXT("MinionEqiupCanvas is null or not found!"));
		return;
	}

	TArray<UUserWidget*> ButtonWidgets;
	UWidgetTree::ForWidgetAndChildren(MinionEqiupCanvas, [&ButtonWidgets](UWidget* Widget)
	{
		UUserWidget* ButtonWidget = Cast<UUserWidget>(Widget);
		if (ButtonWidget && ButtonWidget->GetName().Contains(TEXT("EquipButton")))
		{
			ButtonWidgets.Add(ButtonWidget);
		}
	});
	// So buttons sharing a slot fill in name order (MinionRingEquipButton_1 before _2).
	ButtonWidgets.Sort([](const UUserWidget& A, const UUserWidget& B) { return A.GetName() < B.GetName(); });

	for (UUserWidget* ButtonWidget : ButtonWidgets)
	{
		const TArray<EItemSlot> ItemSlots = GetMinionEquipButtonItemSlots(ButtonWidget->GetName());
		if (ItemSlots.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s doesn't name an item slot, so it won't show any equipment."), *ButtonWidget->GetName());
			continue;
		}

		UButton* Button = GetSingleImageButton(ButtonWidget);
		if (!Button)
		{
			continue;
		}

		UMinionEquipButtonProxy* Proxy = NewObject<UMinionEquipButtonProxy>(this);
		Proxy->ItemSlots = ItemSlots;
		Proxy->ButtonWidget = ButtonWidget;
		Proxy->OwningMenu = this;
		MinionEquipButtonProxies.Add(Proxy);
		Button->OnClicked.AddDynamic(Proxy, &UMinionEquipButtonProxy::HandleClicked);
		// Has to be bound before the Slate widgets are built, which is when it's picked up.
		Button->ToolTipWidgetDelegate.BindDynamic(Proxy, &UMinionEquipButtonProxy::CreateToolTip);
	}

	// No minion is selected yet, so this just empties every slot.
	RefreshMinionEquipButtons();
}

void UCoreMenu::RefreshMinionEquipButtons()
{
	TSet<FString> ShownItemUUIDs;
	for (UMinionEquipButtonProxy* Proxy : MinionEquipButtonProxies)
	{
		TArray<FString> SlotItemUUIDs;
		if (EquipWindowMinion)
		{
			for (const EItemSlot ItemSlot : Proxy->ItemSlots)
			{
				SlotItemUUIDs.Append(EquipWindowMinion->GetEquippedItemUUIDsInSlot(ItemSlot));
			}
		}

		// Buttons sharing a slot (e.g. the two ring buttons) each show the next item in it.
		const FString* ItemUUID = SlotItemUUIDs.FindByPredicate([&ShownItemUUIDs](const FString& UUID) { return !ShownItemUUIDs.Contains(UUID); });
		Proxy->ItemUUID = ItemUUID ? *ItemUUID : FString();
		if (ItemUUID)
		{
			ShownItemUUIDs.Add(*ItemUUID);
		}

		UImage* Icon = GetSingleImageButtonIcon(Proxy->ButtonWidget);
		if (!Icon)
		{
			continue;
		}

		const FBaseItemStruct* ItemRow = Proxy->ItemUUID.IsEmpty() ? nullptr : FindSavedItemRow(Proxy->ItemUUID);
		if (ItemRow && ItemRow->ItemAssetData.ItemIcon)
		{
			Icon->SetBrushFromTexture(ItemRow->ItemAssetData.ItemIcon);
			// Not hit-testable so clicks still reach the owning button.
			Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			continue;
		}

		if (ItemRow)
		{
			UE_LOG(LogTemp, Warning, TEXT("No ItemIcon set for item: %s"), *ItemRow->ItemId.ToString());
		}
		// Without a texture the icon would render as a plain white box. Hidden (not Collapsed) keeps the button's
		// layout size.
		Icon->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMinionEquipButtonProxy::HandleClicked()
{
	if (OwningMenu)
	{
		OwningMenu->OnMinionEquipButtonClicked(ItemUUID);
	}
}

UWidget* UMinionEquipButtonProxy::CreateToolTip()
{
	// An empty slot has no flyout. An equipped item's ItemId comes from its save, so its UUID is enough.
	return OwningMenu && !ItemUUID.IsEmpty() ? OwningMenu->CreateItemToolTip(FString(), ItemUUID) : nullptr;
}

void UCoreMenu::OnMinionEquipButtonClicked(FString ItemUUID)
{
	// An empty slot has nothing to unequip.
	if (ItemUUID.IsEmpty() || !EquipWindowMinion)
	{
		return;
	}

	const FString ItemName = GetSavedItemName(ItemUUID);

	// MinionHandler unequips the item and saves the minion during the broadcast, and the slot buttons catch up through
	// OnEquipWindowMinionSaveDataChanged. The item itself stays in SavedItems.json, so it's back in the stash.
	OnUnequipItemFromMinionEvent.Broadcast(EquipWindowMinion->GetMinionUUID(), ItemUUID);

	if (EquipWindowMinion->IsItemEquipped(ItemUUID))
	{
		SetWarningText(FString::Printf(TEXT("Couldn't remove %s from the selected minion."), *ItemName));
		return;
	}

	const FString Message = FString::Printf(TEXT("Removed %s from the selected minion and returned it to your stash."), *ItemName);
	LogToScreen(Message);
	SetWarningText(Message, 6.f);

	PopulatePlayerStash();
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

	if (IsWindowVisible(ECoreMenuWindow::ShopWindow))
	{
		HideWindow(ECoreMenuWindow::ShopWindow);
		RefreshActiveItemDisplay();
		return;
	}

	ShowWindow(ECoreMenuWindow::ShopWindow);
	PopulateShopGrid();
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
	ShopItemSelectionBorders.Empty();

	UClass* SingleImageButtonClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP_SingleImageButton.WBP_SingleImageButton_C"));
	if (!SingleImageButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_SingleImageButton class."));
		return;
	}

	constexpr int32 NumColumns = 4;
	constexpr float ShopItemSlotSize = 160.f;
	int32 Index = 0;

	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseItem_DT data table is not loaded."));
		return;
	}

	for (int32 ListingIndex = 0; ListingIndex < CurrentShopItems.Num(); ++ListingIndex)
	{
		const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(CurrentShopItems[ListingIndex], TEXT("CoreMenu::PopulateShopGrid"));
		if (!ItemRow || !CurrentShopItemUUIDs.IsValidIndex(ListingIndex))
		{
			continue;
		}
		const FString& ListingUUID = CurrentShopItemUUIDs[ListingIndex];

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
		ItemSlotBox->AddChild(WrapInSelectionBorder(ShopItemWidget, ListingUUID, SelectedItemUUID, ShopItemSelectionBorders));

		UShopItemButtonProxy* Proxy = NewObject<UShopItemButtonProxy>(this);
		Proxy->ItemId = ItemRow->ItemId.ToString();
		Proxy->ItemUUID = ListingUUID;
		Proxy->OwningMenu = this;
		ShopItemButtonProxies.Add(Proxy);
		InnerButton->OnClicked.AddDynamic(Proxy, &UShopItemButtonProxy::HandleClicked);
		// Has to be bound before the grid builds the button's Slate widget, which is when it's picked up.
		InnerButton->ToolTipWidgetDelegate.BindDynamic(Proxy, &UShopItemButtonProxy::CreateToolTip);

		ShopUniGrid->AddChildToUniformGrid(ItemSlotBox, Index / NumColumns, Index % NumColumns);

		++Index;
	}
}

void UCoreMenu::RefreshShopGrid()
{
	PopulateShopGrid();
}

void UCoreMenu::RefreshPlayerStash()
{
	PopulatePlayerStash();
}

void UCoreMenu::RemoveItemFromShop(const FString& ItemId)
{
	// Called by ItemHandler once a purchase has actually succeeded, so a rejected (unaffordable)
	// buy leaves the listing in CurrentShopItems instead of removing it for nothing.
	if (!ItemDataTable)
	{
		return;
	}

	// The bought listing is the selected one, so that's removed when it matches. Otherwise falls back to
	// the first listing with this ItemId. Either way the listing's UUID goes with it, since the bought
	// item now owns that UUID in the stash.
	int32 RemoveIndex = CurrentShopItemUUIDs.IndexOfByKey(SelectedItemUUID);
	const FBaseItemStruct* SelectedRow = CurrentShopItems.IsValidIndex(RemoveIndex)
		? ItemDataTable->FindRow<FBaseItemStruct>(CurrentShopItems[RemoveIndex], TEXT("CoreMenu::RemoveItemFromShop"))
		: nullptr;
	if (!SelectedRow || !SelectedRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
	{
		RemoveIndex = INDEX_NONE;
		for (int32 Index = 0; Index < CurrentShopItems.Num(); ++Index)
		{
			const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(CurrentShopItems[Index], TEXT("CoreMenu::RemoveItemFromShop"));
			if (ItemRow && ItemRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
			{
				RemoveIndex = Index;
				break;
			}
		}
	}

	if (CurrentShopItems.IsValidIndex(RemoveIndex))
	{
		CurrentShopItems.RemoveAt(RemoveIndex);
		if (CurrentShopItemUUIDs.IsValidIndex(RemoveIndex))
		{
			CurrentShopItemUUIDs.RemoveAt(RemoveIndex);
		}
	}

	RefreshShopGrid();
}

void UCoreMenu::OnShopItemButtonClicked(FString ItemId, FString ItemUUID)
{
	UE_LOG(LogTemp, Warning, TEXT("Shop item button clicked for ItemId: %s (UUID: %s)"), *ItemId, *ItemUUID);
	SelectItemData(FText::FromString(ItemId));
	SetSelectedItemUUID(ItemUUID);

	// Lets listeners (e.g. ItemHandler) cache this item's base stats and display them as the
	// active item, matching the stat breakdown shown for a stash selection or randomize.
	OnShopItemSelectedEvent.Broadcast(ItemId, ItemUUID);
}

void UCoreMenu::OnPlayerStashButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerStashButton Clicked."));

	if (IsWindowVisible(ECoreMenuWindow::PlayerStash))
	{
		HideWindow(ECoreMenuWindow::PlayerStash);
		RefreshActiveItemDisplay();
		return;
	}

	ShowWindow(ECoreMenuWindow::PlayerStash);
	PopulatePlayerStash();
}

void UCoreMenu::OnStashCraftButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("StashCraftButton Clicked."));
	if (!ValidateSelectedStashItem())
	{
		return;
	}

	ShowActiveItem();

	CraftStashItem(SelectedItemUUID);
}

void UCoreMenu::OnStashAssignButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("StashAssignButton Clicked."));

	// Shown on every click, whether or not the assignment goes through.
	ShowWindow(ECoreMenuWindow::MinionEquip);

	if (ValidateSelectedStashItem())
	{
		AssignStashItem(SelectedItemUUID);
	}
}

bool UCoreMenu::ValidateSelectedStashItem()
{
	// The selection can also be a shop listing (selecting it doesn't close the stash) or nothing at all.
	TSharedPtr<FJsonObject> ItemObject;
	if (SelectedItemUUID.IsEmpty() || !GetSavedItemJsonByUUID(SelectedItemUUID, ItemObject))
	{
		SetWarningText(TEXT("Select an item in your stash first."));
		return false;
	}
	return true;
}

void UCoreMenu::CraftStashItem(const FString& ItemUUID)
{
	UE_LOG(LogTemp, Warning, TEXT("CraftStashItem not implemented yet (UUID: %s)."), *ItemUUID);
}

void UCoreMenu::AssignStashItem(const FString& ItemUUID)
{
	if (SelectedMinionUUID.IsEmpty())
	{
		SetWarningText(TEXT("Select a minion first."));
		return;
	}

	const FString ItemName = GetSavedItemName(ItemUUID);

	// Read-only, both before and after the broadcast: comparing the minion's saved equipment tells what the
	// assignment took off it, whether that was the item in its slot or one blocked by it (e.g. a shield by a
	// two-handed weapon).
	FMinionSaveData MinionBefore;
	UMinionInstanceManager().GetSavedMinion(SelectedMinionUUID, MinionBefore);
	if (!MinionBefore.bActivePartyMember)
	{
		SetWarningText(TEXT("Minions in the minion stash can't hold items. Select one in your active party."));
		return;
	}

	// MinionHandler equips the item and saves the minion during the broadcast, unequipping whatever was in the
	// item's slot. The item itself stays in SavedItems.json.
	OnAssignItemToMinionEvent.Broadcast(SelectedMinionUUID, ItemUUID);

	FMinionSaveData MinionAfter;
	if (!UMinionInstanceManager().GetSavedMinion(SelectedMinionUUID, MinionAfter) || !MinionAfter.EquippedItemUUIDs.Contains(ItemUUID))
	{
		SetWarningText(FString::Printf(TEXT("Couldn't assign %s to the selected minion."), *ItemName));
		return;
	}

	TArray<FString> RemovedItemNames;
	for (const FString& EquippedItemUUID : MinionBefore.EquippedItemUUIDs)
	{
		if (!MinionAfter.EquippedItemUUIDs.Contains(EquippedItemUUID))
		{
			RemovedItemNames.Add(GetSavedItemName(EquippedItemUUID));
		}
	}

	FString Message = FString::Printf(TEXT("Added %s to the selected minion."), *ItemName);
	if (RemovedItemNames.Num() > 0)
	{
		Message += FString::Printf(TEXT(" Removed %s."), *FString::Join(RemovedItemNames, TEXT(", ")));
	}
	LogToScreen(Message);
	SetWarningText(Message, 6.f);

	// The item has left the stash, and anything it replaced is back in it.
	ClearSelectedItem();
	PopulatePlayerStash();
}

void UCoreMenu::ClearSelectedItem()
{
	SelectedItemData = FBaseItemStruct();
	SelectedItemId.Empty();
	SetSelectedItemUUID(FString());
	bHasSelectedItemData = false;
	SetActiveItemText(TEXT(""));
}

FString UCoreMenu::GetSavedItemName(const FString& ItemUUID) const
{
	if (const FBaseItemStruct* ItemRow = FindSavedItemRow(ItemUUID))
	{
		return ItemRow->ItemName.ToString();
	}

	UE_LOG(LogTemp, Warning, TEXT("No saved item name found for UUID: %s"), *ItemUUID);
	return ItemUUID;
}

const FBaseItemStruct* UCoreMenu::FindSavedItemRow(const FString& ItemUUID) const
{
	TSharedPtr<FJsonObject> ItemObject;
	FString ItemId;
	if (!ItemDataTable || !GetSavedItemJsonByUUID(ItemUUID, ItemObject) || !ItemObject->TryGetStringField(TEXT("ItemId"), ItemId))
	{
		return nullptr;
	}

	for (const FName& RowName : ItemDataTable->GetRowNames())
	{
		const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("CoreMenu::FindSavedItemRow"));
		if (ItemRow && ItemRow->ItemId.ToString().Equals(ItemId, ESearchCase::IgnoreCase))
		{
			return ItemRow;
		}
	}
	return nullptr;
}

void UCoreMenu::OnResetGameButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("ResetGameButton Clicked."));

	// Listeners (ItemHandler, PlayerInventory) delete/clear their own save files
	// (SavedItems.json, SavedCurrency.json) synchronously in response to this broadcast.
	OnResetGameButtonClickedEvent.Broadcast();

	// The previously active/selected item may no longer exist after the reset.
	ClearSelectedItem();
	HideActiveItem();

	PopulatePlayerStash();
	RandomizeShopItems();
}

void UCoreMenu::RegisterWindows()
{
	RegisterWindow(ECoreMenuWindow::ShopWindow, ECoreMenuWindowBucket::MainWindows, { ShopWindowBox });
	RegisterWindow(ECoreMenuWindow::HireShop, ECoreMenuWindowBucket::MainWindows, { HireShopVertBox });
	RegisterWindow(ECoreMenuWindow::MinionEquip, ECoreMenuWindowBucket::MainWindows, { MinionEqiupVertBox });
	RegisterWindow(ECoreMenuWindow::ActiveItemImage, ECoreMenuWindowBucket::MainWindows, { ActiveItemImageHorizBox });

	RegisterWindow(ECoreMenuWindow::PlayerStash, ECoreMenuWindowBucket::SideWindows, { PlayerStashHorizBox });
	RegisterWindow(ECoreMenuWindow::MinionStash, ECoreMenuWindowBucket::SideWindows, { MinionStashVertBox });
	// Only the boxes themselves: this leaves the text's own visibility as WBP_CoreMenu has it for when they come back.
	RegisterWindow(ECoreMenuWindow::ActiveItemText, ECoreMenuWindowBucket::SideWindows,
		{ ActiveItemTextHorizBox, ActiveItemTextVertBox }, /*bIncludeChildren*/ false);

	RegisterWindow(ECoreMenuWindow::Map, ECoreMenuWindowBucket::FullScreen, { MapVertBox });
}

void UCoreMenu::RegisterWindow(ECoreMenuWindow Window, ECoreMenuWindowBucket Bucket, const TArray<UPanelWidget*>& Panels, bool bIncludeChildren)
{
	FCoreMenuWindow& Registered = Windows.Add(Window);
	Registered.Bucket = Bucket;
	Registered.bIncludeChildren = bIncludeChildren;
	for (UPanelWidget* Panel : Panels)
	{
		if (Panel)
		{
			Registered.Panels.Add(Panel);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("A panel for window %s is null or not found!"), *UEnum::GetValueAsString(Window));
		}
	}
}

void UCoreMenu::ShowWindow(ECoreMenuWindow Window)
{
	const FCoreMenuWindow* WindowToShow = Windows.Find(Window);
	if (!WindowToShow)
	{
		UE_LOG(LogTemp, Error, TEXT("Window %s isn't registered."), *UEnum::GetValueAsString(Window));
		return;
	}

	// A FullScreen window shares the screen with no other window, so it hides, and is hidden by, all of them.
	const bool bShowingFullScreen = WindowToShow->Bucket == ECoreMenuWindowBucket::FullScreen;
	for (const TPair<ECoreMenuWindow, FCoreMenuWindow>& Entry : Windows)
	{
		const bool bSharesScreenSpace = bShowingFullScreen || Entry.Value.Bucket == WindowToShow->Bucket
			|| Entry.Value.Bucket == ECoreMenuWindowBucket::FullScreen;
		if (Entry.Key != Window && bSharesScreenSpace)
		{
			SetWindowVisibility(Entry.Value, ESlateVisibility::Hidden);
		}
	}

	SetWindowVisibility(*WindowToShow, ESlateVisibility::Visible);
}

void UCoreMenu::HideWindow(ECoreMenuWindow Window)
{
	if (const FCoreMenuWindow* WindowToHide = Windows.Find(Window))
	{
		SetWindowVisibility(*WindowToHide, ESlateVisibility::Hidden);
	}
}

bool UCoreMenu::IsWindowVisible(ECoreMenuWindow Window) const
{
	const FCoreMenuWindow* Found = Windows.Find(Window);
	return Found && Found->Panels.Num() > 0 && Found->Panels[0] && Found->Panels[0]->IsVisible();
}

bool UCoreMenu::IsAnyWindowVisible(ECoreMenuWindowBucket Bucket) const
{
	for (const TPair<ECoreMenuWindow, FCoreMenuWindow>& Entry : Windows)
	{
		if (Entry.Value.Bucket == Bucket && IsWindowVisible(Entry.Key))
		{
			return true;
		}
	}
	return false;
}

void UCoreMenu::SetWindowVisibility(const FCoreMenuWindow& Window, ESlateVisibility NewVisibility)
{
	for (UPanelWidget* Panel : Window.Panels)
	{
		if (Window.bIncludeChildren)
		{
			SetPanelAndChildrenVisibility(Panel, NewVisibility);
		}
		else if (Panel)
		{
			Panel->SetVisibility(NewVisibility);
		}
	}
}

void UCoreMenu::RefreshActiveItemDisplay()
{
	// Shown now, the active item windows would close the FullScreen window instead of waiting behind it.
	if (!bHasSelectedItemData || IsAnyWindowVisible(ECoreMenuWindowBucket::FullScreen))
	{
		return;
	}

	if (!IsAnyWindowVisible(ECoreMenuWindowBucket::MainWindows))
	{
		UpdateActiveItemImage();
		ShowWindow(ECoreMenuWindow::ActiveItemImage);
	}

	if (!IsAnyWindowVisible(ECoreMenuWindowBucket::SideWindows))
	{
		ShowWindow(ECoreMenuWindow::ActiveItemText);
	}
}

void UCoreMenu::SetSelectedItemUUID(const FString& ItemUUID)
{
	SelectedItemUUID = ItemUUID;
	RefreshSelectionBorders(ShopItemSelectionBorders, SelectedItemUUID);
	RefreshSelectionBorders(StashItemSelectionBorders, SelectedItemUUID);
}

// An outline-only box: the fill stays transparent so it never shows through an icon's transparent
// pixels, and an unselected border's outline is transparent too, so Slate skips drawing it entirely.
static FSlateBrush MakeSelectionBorderBrush(bool bSelected, float Width)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(FLinearColor::Transparent);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(0.f, bSelected ? FLinearColor::White : FLinearColor::Transparent, Width);
	return Brush;
}

UBorder* UCoreMenu::WrapInSelectionBorder(UWidget* Widget, const FString& UUID, const FString& SelectedUUID, TMap<FString, TObjectPtr<UBorder>>& Borders)
{
	UBorder* SelectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());

	// Padding equal to the outline width keeps the widget inside the outline rather than under it.
	SelectionBorder->SetPadding(FMargin(ItemSelectionBorderWidth));
	SelectionBorder->SetHorizontalAlignment(HAlign_Fill);
	SelectionBorder->SetVerticalAlignment(VAlign_Fill);
	SelectionBorder->SetBrush(MakeSelectionBorderBrush(!SelectedUUID.IsEmpty() && UUID == SelectedUUID, ItemSelectionBorderWidth));
	SelectionBorder->SetContent(Widget);

	Borders.Add(UUID, SelectionBorder);
	return SelectionBorder;
}

void UCoreMenu::RefreshSelectionBorders(const TMap<FString, TObjectPtr<UBorder>>& Borders, const FString& SelectedUUID)
{
	for (const TPair<FString, TObjectPtr<UBorder>>& Entry : Borders)
	{
		if (Entry.Value)
		{
			const bool bSelected = !SelectedUUID.IsEmpty() && Entry.Key == SelectedUUID;
			Entry.Value->SetBrush(MakeSelectionBorderBrush(bSelected, ItemSelectionBorderWidth));
		}
	}
}

void UCoreMenu::ShowActiveItem()
{
	UpdateActiveItemImage();
	ShowWindow(ECoreMenuWindow::ActiveItemImage);
	ShowWindow(ECoreMenuWindow::ActiveItemText);
}

void UCoreMenu::HideActiveItem()
{
	HideWindow(ECoreMenuWindow::ActiveItemImage);
	HideWindow(ECoreMenuWindow::ActiveItemText);
}

void UCoreMenu::UpdateActiveItemImage()
{
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

void UCoreMenu::OnBattlePlayButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("BattlePlayButton Clicked."));
	OnBattlePlayButtonClickedEvent.Broadcast();
}

void UCoreMenu::SetBattleTimerText(float BattleTime)
{
	if (!BattleTimerText)
	{
		return;
	}

	const int32 Minutes = FMath::FloorToInt(BattleTime / 60.f);
	const float Seconds = BattleTime - Minutes * 60.f;
	BattleTimerText->SetText(FText::FromString(FString::Printf(TEXT("%d:%04.1f"), Minutes, Seconds)));
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
	CurrentShopItemUUIDs.Empty();

	if (ItemDataTableRowCount <= 0 || ItemDataTableRowNames.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("RandomizeShopItems: BaseItem_DT has no rows loaded; skipping."));
		return;
	}

	ShopItemCount = FMath::RandRange(6,12);

	// Every roll lists at least one weapon and one armor piece, picked first so the random fill can't skip them.
	TArray<FName> WeaponRowNames;
	TArray<FName> ArmorRowNames;
	if (ItemDataTable)
	{
		for (const FName& RowName : ItemDataTableRowNames)
		{
			const FBaseItemStruct* ItemRow = ItemDataTable->FindRow<FBaseItemStruct>(RowName, TEXT("CoreMenu::RandomizeShopItems"));
			if (!ItemRow)
			{
				continue;
			}
			if (ItemRow->ItemClass == EItemClass::Weapon)
			{
				WeaponRowNames.Add(RowName);
			}
			else if (ItemRow->ItemClass == EItemClass::Armor)
			{
				ArmorRowNames.Add(RowName);
			}
		}
	}

	if (WeaponRowNames.Num() > 0)
	{
		CurrentShopItems.Add(WeaponRowNames[FMath::RandRange(0, WeaponRowNames.Num() - 1)]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RandomizeShopItems: BaseItem_DT has no Weapon rows; shop can't guarantee a weapon."));
	}

	if (ArmorRowNames.Num() > 0)
	{
		CurrentShopItems.Add(ArmorRowNames[FMath::RandRange(0, ArmorRowNames.Num() - 1)]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RandomizeShopItems: BaseItem_DT has no Armor rows; shop can't guarantee an armor piece."));
	}

	while (CurrentShopItems.Num() < ShopItemCount)
	{
		int32 RandomItemRowName = FMath::RandRange(1, ItemDataTableRowCount);
		RandomItemRowName -= 1;
		CurrentShopItems.Add(ItemDataTableRowNames[RandomItemRowName]);
	}

	// Otherwise the guaranteed weapon/armor would always sit in the first two slots.
	Algo::RandomShuffle(CurrentShopItems);

	for (int32 i = 0; i < CurrentShopItems.Num(); i++)
	{
		// Minted now rather than when Buy is clicked, so every listing has one from the moment it's shown.
		CurrentShopItemUUIDs.Add(FGuid::NewGuid().ToString());
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

void UCoreMenu::SetWarningText(const FString& NewMessage, float DisplaySeconds)
{
	if (!WarningsTextBox)
	{
		UE_LOG(LogTemp, Error, TEXT("WarningsTextBox is null or not found!"));
		return;
	}

	// A new warning replaces one that's mid-fade, so it starts back at full opacity on its own clock.
	WarningsTextBox->SetText(FText::FromString(NewMessage));
	WarningsTextBox->SetRenderOpacity(1.f);
	WarningTextAge = 0.f;
	WarningTextDisplaySeconds = NewMessage.IsEmpty() ? 0.f : DisplaySeconds;
}

void UCoreMenu::TickWarningText(float DeltaTime)
{
	if (!WarningsTextBox || WarningTextDisplaySeconds <= 0.f)
	{
		return;
	}

	WarningTextAge += DeltaTime;
	const float FadeAge = WarningTextAge - WarningTextDisplaySeconds;
	if (FadeAge >= WarningTextFadeSeconds)
	{
		WarningsTextBox->SetText(FText::GetEmpty());
		WarningsTextBox->SetRenderOpacity(1.f);
		WarningTextDisplaySeconds = 0.f;
	}
	else if (FadeAge > 0.f)
	{
		WarningsTextBox->SetRenderOpacity(1.f - FadeAge / WarningTextFadeSeconds);
	}
}

void UCoreMenu::SetSelectedMinion(UPlayerMinion* Minion)
{
	if (SelectedMinion)
	{
		SelectedMinion->OnHealthChangedEvent.RemoveDynamic(this, &UCoreMenu::OnSelectedMinionHealthChanged);
		SelectedMinion->OnHitTakenEvent.RemoveDynamic(this, &UCoreMenu::OnSelectedMinionHitTaken);
	}

	// Numbers still fading belong to the old minion.
	ClearDamageNumbers(MinionDamageNumbers);

	SelectedMinion = Minion;
	if (!SelectedMinion)
	{
		ClearHealthDisplay(BattleMinionHPBar, MinionHPBarDefaultColor);
		return;
	}

	SelectedMinion->OnHealthChangedEvent.AddDynamic(this, &UCoreMenu::OnSelectedMinionHealthChanged);
	SelectedMinion->OnHitTakenEvent.AddDynamic(this, &UCoreMenu::OnSelectedMinionHitTaken);
	SetHealthDisplay(BattleMinionHPBar, MinionHPBarDefaultColor,
		SelectedMinion->GetCurrentHealth(), SelectedMinion->GetMaxHealth(), SelectedMinion->GetCurrentOvershield());
}

void UCoreMenu::SetBattleEnemies(const TArray<UEnemy*>& Enemies)
{
	// The previous group's bars go, along with their bindings and any of their damage numbers still fading.
	for (UEnemyHPBarProxy* Proxy : EnemyHPBarProxies)
	{
		if (Proxy && Proxy->Enemy)
		{
			Proxy->Enemy->OnHealthChangedEvent.RemoveDynamic(Proxy, &UEnemyHPBarProxy::HandleHealthChanged);
			Proxy->Enemy->OnHitTakenEvent.RemoveDynamic(this, &UCoreMenu::OnBattleEnemyHitTaken);
		}
	}
	EnemyHPBarProxies.Reset();
	ClearDamageNumbers(EnemyDamageNumbers);

	if (!EnemyHPBarsBox || !BattleEnemyHPBar)
	{
		UE_LOG(LogTemp, Error, TEXT("No EnemyHPBarsBox/BattleEnemyHPBar to show enemy health in."));
		return;
	}
	EnemyHPBarsBox->ClearChildren();

	// Each bar takes the size and alignment BattleEnemyHPBar had in its own slot, which the box took over (so bars
	// that filled their space share it between them).
	const UVerticalBoxSlot* TemplateSlot = Cast<UVerticalBoxSlot>(EnemyHPBarsBox->Slot);

	for (UEnemy* Enemy : Enemies)
	{
		if (!Enemy)
		{
			continue;
		}

		// Made from BattleEnemyHPBar as a template, so it keeps the style and fill color set in WBP_CoreMenu.
		UProgressBar* HPBar = NewObject<UProgressBar>(WidgetTree, BattleEnemyHPBar->GetClass(), NAME_None, RF_Transactional, BattleEnemyHPBar);
		if (UVerticalBoxSlot* BarSlot = EnemyHPBarsBox->AddChildToVerticalBox(HPBar))
		{
			if (TemplateSlot)
			{
				BarSlot->SetSize(TemplateSlot->GetSize());
				BarSlot->SetHorizontalAlignment(TemplateSlot->GetHorizontalAlignment());
				BarSlot->SetVerticalAlignment(TemplateSlot->GetVerticalAlignment());
			}
			BarSlot->SetPadding(FMargin(0.f, EnemyHPBarProxies.Num() > 0 ? EnemyHPBarSpacing : 0.f, 0.f, 0.f));
		}

		UEnemyHPBarProxy* Proxy = NewObject<UEnemyHPBarProxy>(this);
		Proxy->Enemy = Enemy;
		Proxy->HPBar = HPBar;
		Proxy->OwningMenu = this;
		EnemyHPBarProxies.Add(Proxy);

		Enemy->OnHealthChangedEvent.AddDynamic(Proxy, &UEnemyHPBarProxy::HandleHealthChanged);
		Enemy->OnHitTakenEvent.AddDynamic(this, &UCoreMenu::OnBattleEnemyHitTaken);
		SetHealthDisplay(HPBar, EnemyHPBarDefaultColor, Enemy->GetCurrentHealth(), Enemy->GetMaxHealth(), Enemy->GetCurrentOvershield());
	}
}

void UEnemyHPBarProxy::HandleHealthChanged(float CurrentHealth, float MaxHealth, float CurrentOvershield)
{
	if (OwningMenu)
	{
		OwningMenu->SetHealthDisplay(HPBar, OwningMenu->EnemyHPBarDefaultColor, CurrentHealth, MaxHealth, CurrentOvershield);
	}
}

void UCoreMenu::OnSelectedMinionHealthChanged(float CurrentHealth, float MaxHealth, float CurrentOvershield)
{
	SetHealthDisplay(BattleMinionHPBar, MinionHPBarDefaultColor, CurrentHealth, MaxHealth, CurrentOvershield);
}

void UCoreMenu::OnSelectedMinionHitTaken(const FCombatHitResult& HitResult)
{
	ShowDamageNumber(MinionDamageNumbers, HitResult);
}

void UCoreMenu::OnBattleEnemyHitTaken(const FCombatHitResult& HitResult)
{
	ShowDamageNumber(EnemyDamageNumbers, HitResult);
}

void UCoreMenu::SetHealthDisplay(UProgressBar* HPBar, const FLinearColor& UnshieldedBarColor,
	float CurrentHealth, float MaxHealth, float CurrentOvershield)
{
	if (MaxHealth <= 0.f)
	{
		ClearHealthDisplay(HPBar, UnshieldedBarColor);
		return;
	}

	// Overshield is added to both sides (e.g. 25 health + 15 shield fills the bar), and the bar stays
	// ShieldedHPBarColor until the shield is used up and the total is back down to MaxHealth.
	const float Overshield = FMath::Max(0.f, CurrentOvershield);
	const float DisplayedHealth = CurrentHealth + Overshield;
	const float DisplayedMaxHealth = MaxHealth + Overshield;

	if (HPBar)
	{
		HPBar->SetPercent(FMath::Clamp(DisplayedHealth / DisplayedMaxHealth, 0.f, 1.f));
		HPBar->SetFillColorAndOpacity(Overshield > 0.f ? ShieldedHPBarColor : UnshieldedBarColor);
	}
}

void UCoreMenu::ClearHealthDisplay(UProgressBar* HPBar, const FLinearColor& UnshieldedBarColor)
{
	if (HPBar)
	{
		HPBar->SetPercent(0.f);
		HPBar->SetFillColorAndOpacity(UnshieldedBarColor);
	}
}

UVerticalBox* UCoreMenu::SwapForVerticalBox(UWidget* Widget)
{
	UPanelWidget* Parent = Widget ? Widget->GetParent() : nullptr;
	if (!Parent)
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no parent panel to swap a vertical box into."), Widget ? *Widget->GetName() : TEXT("Widget"));
		return nullptr;
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The box takes over Widget's index and a copy of its slot (alignment, padding, anchors, ...), so it sits
	// exactly where Widget did. Removing Widget first also frees up single-child panels (SizeBox, Border, ...).
	// InsertChildAt only reorders the UMG side, hence running before Slate is built.
	UPanelSlot* WidgetSlot = Widget->Slot;
	const int32 Index = Parent->GetChildIndex(Widget);
	Parent->RemoveChildAt(Index);
	Parent->InsertChildAt(Index, Box, WidgetSlot);
	return Box;
}

void UCoreMenu::InitDamageNumberStack(UTextBlock* HPText, FDamageNumberStack& Stack)
{
	Stack.Box = SwapForVerticalBox(HPText);
	Stack.Template = HPText;
}

void UCoreMenu::ShowDamageNumber(FDamageNumberStack& Stack, const FCombatHitResult& HitResult)
{
	const float Damage = HitResult.HealthDamage + HitResult.OvershieldDamage;
	if (!Stack.Box || Damage <= 0.f)
	{
		return;
	}

	// Made from the HP text as a template, so it keeps the font, color, shadow and justification set in
	// WBP_CoreMenu.
	UTextBlock* NumberText = NewObject<UTextBlock>(WidgetTree, Stack.Template->GetClass(), NAME_None, RF_Transactional, Stack.Template);
	// Rounded up, so a hit that only took a sliver still reads as at least 1.
	NumberText->SetText(FText::AsNumber(FMath::CeilToInt(Damage)));
	NumberText->SetVisibility(ESlateVisibility::HitTestInvisible);
	NumberText->SetRenderOpacity(1.f);

	// Added at the bottom, so the oldest numbers stay on top.
	Stack.Box->AddChildToVerticalBox(NumberText);
	Stack.Numbers.AddDefaulted_GetRef().Text = NumberText;
}

void UCoreMenu::TickDamageNumbers(FDamageNumberStack& Stack, float DeltaTime)
{
	for (int32 Index = Stack.Numbers.Num() - 1; Index >= 0; --Index)
	{
		FDamageNumber& Number = Stack.Numbers[Index];
		Number.Age += DeltaTime;
		if (Number.Age >= DamageNumberLifetime)
		{
			Number.Text->RemoveFromParent();
			Stack.Numbers.RemoveAt(Index);
		}
		else
		{
			Number.Text->SetRenderOpacity(1.f - Number.Age / DamageNumberLifetime);
		}
	}
}

void UCoreMenu::ClearDamageNumbers(FDamageNumberStack& Stack)
{
	if (Stack.Box)
	{
		Stack.Box->ClearChildren();
	}
	Stack.Numbers.Reset();
}


