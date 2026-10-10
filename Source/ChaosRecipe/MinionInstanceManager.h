// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.h"
#include "MinionClassStruct.h"
#include "MinionInstanceManager.generated.h"

// Everything SavedMinions.json keeps for one minion. Health isn't saved, since every battle restores it to full.
USTRUCT(BlueprintType)
struct FMinionSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion")
    FString MinionUUID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion")
    EMinionClass MinionClass = EMinionClass::Warrior;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion")
    int32 Level = 1;

    // The minion's own attributes at Level, before equipped items.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion")
    FCombatAttributes BaseAttributes;

    // SavedItems.json UUIDs of every item it has equipped, re-equipped from the stash when it's loaded.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minion")
    TArray<FString> EquippedItemUUIDs;
};

// Saves/loads minions to/from SavedMinions.json in the project's Saved directory, keyed by MinionUUID, the
// same way UItemInstanceManager does for items in SavedItems.json.
class CHAOSRECIPE_API UMinionInstanceManager
{
public:
    UMinionInstanceManager();

    void SaveMinion(const FMinionSaveData& MinionData);

    bool GetSavedMinion(const FString& MinionUUID, FMinionSaveData& OutMinionData) const;
    bool HasSavedMinion(const FString& MinionUUID) const;
    void RemoveSavedMinion(const FString& MinionUUID);

    // Clears every saved minion from memory and deletes SavedMinions.json from disk (e.g. for a game reset).
    void ClearAllSavedMinions();

    const TMap<FString, FMinionSaveData>& GetSavedMinions() const
    {
        return SavedMinionsByUUID;
    }

    // The MinionUUID of the saved minion with ItemUUID equipped, or empty if none has it.
    FString FindMinionForItem(const FString& ItemUUID) const;

    // Drops ItemUUID from a saved minion's equipped items, for a minion that isn't loaded to unequip it itself.
    bool RemoveEquippedItem(const FString& MinionUUID, const FString& ItemUUID);

private:
    void LoadSavedMinionsFromDisk();
    void WriteSavedMinionsToDisk() const;

    TMap<FString, FMinionSaveData> SavedMinionsByUUID;
};
