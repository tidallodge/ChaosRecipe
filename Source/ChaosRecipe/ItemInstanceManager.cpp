// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemInstanceManager.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FString SavedItemsFileName = TEXT("SavedItems.json");
}

namespace
{
    TSharedPtr<FJsonObject> CreateWeaponJsonObject(const FString& ItemUUID, const FItemWeaponStatsStruct& ItemData)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        TSharedRef<FJsonObject> JsonObjectRef = JsonObject.ToSharedRef();

        FJsonObjectConverter::UStructToJsonObject(FItemWeaponStatsStruct::StaticStruct(), &ItemData, JsonObjectRef, 0, 0);

        JsonObject->SetStringField(TEXT("UUID"), ItemUUID);
        JsonObject->SetStringField(TEXT("ItemId"), ItemData.ItemId.ToString());
        JsonObject->SetStringField(TEXT("ItemType"), TEXT("Weapon"));
        return JsonObject;
    }

    TSharedPtr<FJsonObject> CreateArmorJsonObject(const FString& ItemUUID, const FItemArmorStatsStruct& ItemData)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        TSharedRef<FJsonObject> JsonObjectRef = JsonObject.ToSharedRef();

        FJsonObjectConverter::UStructToJsonObject(FItemArmorStatsStruct::StaticStruct(), &ItemData, JsonObjectRef, 0, 0);

        JsonObject->SetStringField(TEXT("UUID"), ItemUUID);
        JsonObject->SetStringField(TEXT("ItemId"), ItemData.ItemId.ToString());
        JsonObject->SetStringField(TEXT("ItemType"), TEXT("Armor"));
        return JsonObject;
    }
}

ItemInstanceManager::ItemInstanceManager()
{
}

ItemInstanceManager::~ItemInstanceManager()
{
}

void ItemInstanceManager::SaveItem(const FString& ItemUUID, const FItemWeaponStatsStruct& ItemData)
{
    if (ItemUUID.IsEmpty())
    {
        return;
    }

    SavedItemsByUUID.Add(ItemUUID, CreateWeaponJsonObject(ItemUUID, ItemData));
    WriteSavedItemsToDisk();
}

void ItemInstanceManager::SaveItem(const FString& ItemUUID, const FItemArmorStatsStruct& ItemData)
{
    if (ItemUUID.IsEmpty())
    {
        return;
    }

    SavedItemsByUUID.Add(ItemUUID, CreateArmorJsonObject(ItemUUID, ItemData));
    WriteSavedItemsToDisk();
}

void ItemInstanceManager::WriteSavedItemsToDisk() const
{
    TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> ItemsArray;
    for (const TPair<FString, TSharedPtr<FJsonObject>>& Pair : SavedItemsByUUID)
    {
        ItemsArray.Add(MakeShared<FJsonValueObject>(Pair.Value.ToSharedRef()));
    }
    RootObject->SetArrayField(TEXT("Items"), ItemsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject, Writer);

    const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedItemsFileName;
    FFileHelper::SaveStringToFile(OutputString, *SaveFilePath);
}

TSharedPtr<FJsonObject> ItemInstanceManager::GetSavedItemJson(const FString& ItemUUID) const
{
    const TSharedPtr<FJsonObject>* FoundItem = SavedItemsByUUID.Find(ItemUUID);
    return FoundItem ? *FoundItem : nullptr;
}

bool ItemInstanceManager::HasSavedItem(const FString& ItemUUID) const
{
    return SavedItemsByUUID.Contains(ItemUUID);
}

void ItemInstanceManager::RemoveSavedItem(const FString& ItemUUID)
{
    SavedItemsByUUID.Remove(ItemUUID);
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
