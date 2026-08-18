// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemInstanceManager.h"

ItemInstanceManager::ItemInstanceManager()
{
}

ItemInstanceManager::~ItemInstanceManager()
{
}

void ItemInstanceManager::AssignItemToEntity(const FString& ItemUUID, EAssignedEntity AssignedEntity)
{
    if (ItemUUID.IsEmpty())
    {
        return;
    }

    ItemUUIDToAssignedEntity.FindOrAdd(ItemUUID) = AssignedEntity;
}

EAssignedEntity ItemInstanceManager::GetAssignedEntityForItem(const FString& ItemUUID) const
{
    const EAssignedEntity* FoundEntity = ItemUUIDToAssignedEntity.Find(ItemUUID);
    return FoundEntity ? *FoundEntity : EAssignedEntity::None;
}

bool ItemInstanceManager::HasItemAssignment(const FString& ItemUUID) const
{
    return ItemUUIDToAssignedEntity.Contains(ItemUUID);
}

void ItemInstanceManager::RemoveItemAssignment(const FString& ItemUUID)
{
    ItemUUIDToAssignedEntity.Remove(ItemUUID);
}
