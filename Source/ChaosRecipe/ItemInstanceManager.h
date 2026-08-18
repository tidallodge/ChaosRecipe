// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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

    void AssignItemToEntity(const FString& ItemUUID, EAssignedEntity AssignedEntity);
    EAssignedEntity GetAssignedEntityForItem(const FString& ItemUUID) const;
    bool HasItemAssignment(const FString& ItemUUID) const;
    void RemoveItemAssignment(const FString& ItemUUID);

    const TMap<FString, EAssignedEntity>& GetAssignedItems() const
    {
        return ItemUUIDToAssignedEntity;
    }

private:
    TMap<FString, EAssignedEntity> ItemUUIDToAssignedEntity;
};
