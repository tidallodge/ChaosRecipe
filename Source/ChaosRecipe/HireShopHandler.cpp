// HireShopHandler
/** Purpose: Keep the list of minions the hire shop offers, saved between sessions in SavedHireShop.json
    Reroll that list for gold when CoreMenu's HireShopRerollButton is clicked
*/

#include "HireShopHandler.h"
#include "CoreMenu.h"
#include "PlayerInventory.h"
#include "DataTableHelpers.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* MinionClassDataTablePath = TEXT("/Game/MinionData/MinionClass_DT.MinionClass_DT");
	const FString SavedHireShopFileName = TEXT("SavedHireShop.json");
}

void UHireShopHandler::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{
	if (!CoreMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("no CoreMenu for HireShopHandler"));
		return;
	}

	BoundCoreMenu = CoreMenu;
	CoreMenu->OnHireShopRerollButtonClickedEvent.AddDynamic(this, &UHireShopHandler::OnRerollHireShop);
	CoreMenu->OnResetGameButtonClickedEvent.AddDynamic(this, &UHireShopHandler::OnResetGame);

	// Nothing is saved yet in a first session, so it starts with a free roll.
	if (!LoadHireShopMinions() && RollHireShopMinions())
	{
		SaveHireShopMinions();
	}
	UpdateHireShopDisplay();
}

void UHireShopHandler::BindToPlayerInventory(UPlayerInventory* PlayerInventory)
{
	if (!PlayerInventory)
	{
		UE_LOG(LogTemp, Error, TEXT("no PlayerInventory for HireShopHandler"));
		return;
	}

	BoundPlayerInventory = PlayerInventory;
}

void UHireShopHandler::OnRerollHireShop()
{
	if (BoundPlayerInventory && !BoundPlayerInventory->CanAffordGoldCost(RerollGoldCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("HireShopHandler: Player does not have enough gold to reroll the hire shop (cost=%d)."), RerollGoldCost);
		if (BoundCoreMenu)
		{
			BoundCoreMenu->SetWarningText(TEXT("Not enough gold to complete action!"));
		}
		return;
	}

	if (!RollHireShopMinions())
	{
		return;
	}

	if (BoundPlayerInventory)
	{
		BoundPlayerInventory->AdjustPlayerGoldCount(-RerollGoldCost);
	}

	SaveHireShopMinions();
	UpdateHireShopDisplay();
}

void UHireShopHandler::OnResetGame()
{
	RollHireShopMinions();
	SaveHireShopMinions();
	UpdateHireShopDisplay();
}

bool UHireShopHandler::RollHireShopMinions()
{
	TArray<FMinionClassStruct> MinionClassRows;
	if (!DataTableHelpers::LoadAllRows(MinionClassDataTablePath, MinionClassRows))
	{
		UE_LOG(LogTemp, Error, TEXT("HireShopHandler: MinionClass_DT has no rows to roll the hire shop from."));
		return false;
	}

	HireShopMinions.Reset();
	HireShopMinionUUIDs.Reset();
	for (int32 Index = 0; Index < MaxHireShopMinions; ++Index)
	{
		HireShopMinions.Add(MinionClassRows[FMath::RandRange(0, MinionClassRows.Num() - 1)].MinionClass);
		HireShopMinionUUIDs.Add(FGuid::NewGuid().ToString());
	}
	return true;
}

bool UHireShopHandler::LoadHireShopMinions()
{
	FString SaveString;
	if (!FFileHelper::LoadFileToString(SaveString, *(FPaths::ProjectSavedDir() / SavedHireShopFileName)))
	{
		return false;
	}

	FHireShopSaveData SaveData;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(SaveString, &SaveData))
	{
		UE_LOG(LogTemp, Warning, TEXT("HireShopHandler: Failed to parse %s."), *SavedHireShopFileName);
		return false;
	}

	HireShopMinions = SaveData.MinionClasses;
	HireShopMinionUUIDs = SaveData.MinionUUIDs;

	// A save from before listings had UUIDs has none, so the listings missing one get one now, saved so they keep it.
	if (HireShopMinionUUIDs.Num() != HireShopMinions.Num())
	{
		HireShopMinionUUIDs.SetNum(HireShopMinions.Num());
		for (FString& MinionUUID : HireShopMinionUUIDs)
		{
			if (MinionUUID.IsEmpty())
			{
				MinionUUID = FGuid::NewGuid().ToString();
			}
		}
		SaveHireShopMinions();
	}
	return true;
}

void UHireShopHandler::SaveHireShopMinions() const
{
	FHireShopSaveData SaveData;
	SaveData.MinionClasses = HireShopMinions;
	SaveData.MinionUUIDs = HireShopMinionUUIDs;

	FString SaveString;
	if (FJsonObjectConverter::UStructToJsonObjectString(SaveData, SaveString))
	{
		FFileHelper::SaveStringToFile(SaveString, *(FPaths::ProjectSavedDir() / SavedHireShopFileName));
	}
}

void UHireShopHandler::UpdateHireShopDisplay() const
{
	if (BoundCoreMenu)
	{
		BoundCoreMenu->SetHireShopMinions(HireShopMinions, HireShopMinionUUIDs);
	}
}
