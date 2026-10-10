// MinionHandler
/** Purpose: Create, load and track the player's minions by their MinionUUID
    Save a minion to SavedMinions.json whenever its level, attributes or equipped items change
    Keep each item equipped by at most one minion
*/

#include "MinionHandler.h"
#include "PlayerMinion.h"
#include "CoreMenu.h"
#include "ItemHandler.h"

void UMinionHandler::BindToCoreMenuEvents(UCoreMenu* CoreMenu)
{
	if (!CoreMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("no CoreMenu for MinionHandler"));
		return;
	}

	BoundCoreMenu = CoreMenu;
	CoreMenu->OnResetGameButtonClickedEvent.AddDynamic(this, &UMinionHandler::OnResetGame);
	UpdateMinionButtons();
}

void UMinionHandler::BindToItemHandlerEvents(UItemHandler* ItemHandler)
{
	if (!ItemHandler)
	{
		UE_LOG(LogTemp, Error, TEXT("no ItemHandler for MinionHandler"));
		return;
	}

	BoundItemHandler = ItemHandler;
	for (const TPair<FString, TObjectPtr<UPlayerMinion>>& Entry : MinionsByUUID)
	{
		Entry.Value->BindToItemHandlerEvents(ItemHandler);
	}
}

UPlayerMinion* UMinionHandler::CreateMinion(EMinionClass InMinionClass, int32 InLevel)
{
	UPlayerMinion* Minion = NewObject<UPlayerMinion>(this);
	if (!Minion->InitializeMinion(InMinionClass, InLevel))
	{
		return nullptr;
	}

	RegisterMinion(Minion);
	UE_LOG(LogTemp, Warning, TEXT("MinionHandler: Created minion %s."), *Minion->GetMinionUUID());
	return Minion;
}

UPlayerMinion* UMinionHandler::LoadMinion(const FString& MinionUUID)
{
	if (UPlayerMinion* LoadedMinion = FindMinion(MinionUUID))
	{
		return LoadedMinion;
	}

	FMinionSaveData SaveData;
	if (!SavedMinionsManager.GetSavedMinion(MinionUUID, SaveData))
	{
		UE_LOG(LogTemp, Warning, TEXT("MinionHandler: No saved minion with UUID '%s' to load."), *MinionUUID);
		return nullptr;
	}

	UPlayerMinion* Minion = NewObject<UPlayerMinion>(this);
	if (!Minion->InitializeFromSaveData(SaveData))
	{
		return nullptr;
	}

	// Registering re-saves it, which drops any saved item it couldn't re-equip (e.g. one sold since).
	RegisterMinion(Minion);
	return Minion;
}

TArray<UPlayerMinion*> UMinionHandler::LoadSavedMinions()
{
	// Copied first, since loading a minion re-saves it into the map this would otherwise be iterating.
	TArray<FString> MinionUUIDs;
	SavedMinionsManager.GetSavedMinions().GetKeys(MinionUUIDs);

	TArray<UPlayerMinion*> LoadedMinions;
	for (const FString& MinionUUID : MinionUUIDs)
	{
		if (UPlayerMinion* Minion = LoadMinion(MinionUUID))
		{
			LoadedMinions.Add(Minion);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("MinionHandler: Loaded %d of %d saved minion(s)."), LoadedMinions.Num(), MinionUUIDs.Num());
	return LoadedMinions;
}

UPlayerMinion* UMinionHandler::FindMinion(const FString& MinionUUID) const
{
	const TObjectPtr<UPlayerMinion>* FoundMinion = MinionsByUUID.Find(MinionUUID);
	return FoundMinion ? FoundMinion->Get() : nullptr;
}

TArray<UPlayerMinion*> UMinionHandler::GetMinions() const
{
	TArray<UPlayerMinion*> Minions;
	for (const TPair<FString, TObjectPtr<UPlayerMinion>>& Entry : MinionsByUUID)
	{
		Minions.Add(Entry.Value);
	}
	return Minions;
}

bool UMinionHandler::EquipItemOnMinion(const FString& MinionUUID, const FString& ItemUUID)
{
	UPlayerMinion* Minion = FindMinion(MinionUUID);
	if (!Minion)
	{
		UE_LOG(LogTemp, Warning, TEXT("MinionHandler: No loaded minion with UUID '%s' to equip %s on."), *MinionUUID, *ItemUUID);
		return false;
	}

	// Looked up before equipping, since afterward both minions' saves would list the item.
	const FString PreviousMinionUUID = FindMinionForItem(ItemUUID);
	if (!Minion->EquipItem(ItemUUID))
	{
		return false;
	}

	if (!PreviousMinionUUID.IsEmpty() && PreviousMinionUUID != MinionUUID)
	{
		UnequipItemFromMinion(PreviousMinionUUID, ItemUUID);
		UE_LOG(LogTemp, Warning, TEXT("MinionHandler: Moved %s from minion %s to minion %s."), *ItemUUID, *PreviousMinionUUID, *MinionUUID);
	}
	return true;
}

bool UMinionHandler::UnequipItemFromMinion(const FString& MinionUUID, const FString& ItemUUID)
{
	if (UPlayerMinion* Minion = FindMinion(MinionUUID))
	{
		return Minion->UnequipItem(ItemUUID);
	}

	// Not loaded this session, so edit its save directly or it would re-equip the item when it's loaded.
	return SavedMinionsManager.RemoveEquippedItem(MinionUUID, ItemUUID);
}

FString UMinionHandler::FindMinionForItem(const FString& ItemUUID) const
{
	// Every equipment change on a loaded minion is saved straight away, so the saves cover loaded and
	// unloaded minions alike.
	return SavedMinionsManager.FindMinionForItem(ItemUUID);
}

bool UMinionHandler::SetMinionLevel(const FString& MinionUUID, int32 NewLevel)
{
	UPlayerMinion* Minion = FindMinion(MinionUUID);
	if (!Minion)
	{
		UE_LOG(LogTemp, Warning, TEXT("MinionHandler: No loaded minion with UUID '%s' to set the level of."), *MinionUUID);
		return false;
	}

	Minion->SetLevel(NewLevel);
	return true;
}

void UMinionHandler::OnResetGame()
{
	SavedMinionsManager.ClearAllSavedMinions();

	for (const TPair<FString, TObjectPtr<UPlayerMinion>>& Entry : MinionsByUUID)
	{
		UPlayerMinion* Minion = Entry.Value;
		Minion->UnequipAll();
		Minion->InitializeMinion(Minion->GetMinionClass());
		SaveMinion(Minion);
	}

	UE_LOG(LogTemp, Warning, TEXT("MinionHandler: Reset - %d minion(s) back to level 1 with nothing equipped."), MinionsByUUID.Num());
}

void UMinionHandler::HandleMinionSaveDataChanged(FString MinionUUID)
{
	if (const UPlayerMinion* Minion = FindMinion(MinionUUID))
	{
		SaveMinion(Minion);
	}
}

void UMinionHandler::RegisterMinion(UPlayerMinion* Minion)
{
	MinionsByUUID.Add(Minion->GetMinionUUID(), Minion);
	Minion->OnSaveDataChangedEvent.AddDynamic(this, &UMinionHandler::HandleMinionSaveDataChanged);
	if (BoundItemHandler)
	{
		Minion->BindToItemHandlerEvents(BoundItemHandler);
	}

	SaveMinion(Minion);
	UpdateMinionButtons();
}

void UMinionHandler::SaveMinion(const UPlayerMinion* Minion)
{
	SavedMinionsManager.SaveMinion(Minion->GetSaveData());
}

void UMinionHandler::UpdateMinionButtons() const
{
	if (BoundCoreMenu)
	{
		BoundCoreMenu->SetMinionButtons(GetMinions());
	}
}
