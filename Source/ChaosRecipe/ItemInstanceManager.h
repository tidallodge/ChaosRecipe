// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FItemWeaponStatsStruct;
struct FItemArmorStatsStruct;

UENUM(BlueprintType)
enum class EAssignedEntity : uint8
{
    None UMETA(DisplayName = "None"),
    Player UMETA(DisplayName = "Player"),
    NPC UMETA(DisplayName = "NPC"),
    Enemy UMETA(DisplayName = "Enemy"),
    World UMETA(DisplayName = "World")
};

class CHAOSRECIPE_API ItemInstanceManager
{
public:
    ItemInstanceManager();
    ~ItemInstanceManager();

    void SaveItem(const FString& ItemUUID, const FItemWeaponStatsStruct& ItemData);
    void SaveItem(const FString& ItemUUID, const FItemArmorStatsStruct& ItemData);

    TSharedPtr<FJsonObject> GetSavedItemJson(const FString& ItemUUID) const;
    bool HasSavedItem(const FString& ItemUUID) const;
    void RemoveSavedItem(const FString& ItemUUID);

    const TMap<FString, TSharedPtr<FJsonObject>>& GetSavedItems() const
    {
        return SavedItemsByUUID;
    }

    void AssignItemToEntity(const FString& ItemUUID, EAssignedEntity AssignedEntity);
    EAssignedEntity GetAssignedEntityForItem(const FString& ItemUUID) const;
    bool HasItemAssignment(const FString& ItemUUID) const;
    void RemoveItemAssignment(const FString& ItemUUID);

    const TMap<FString, EAssignedEntity>& GetAssignedItems() const
    {
        return ItemUUIDToAssignedEntity;
    }

private:
    void LoadSavedItemsFromDisk();
    void WriteSavedItemsToDisk() const;

    TMap<FString, TSharedPtr<FJsonObject>> SavedItemsByUUID;
    TMap<FString, EAssignedEntity> ItemUUIDToAssignedEntity;
};
