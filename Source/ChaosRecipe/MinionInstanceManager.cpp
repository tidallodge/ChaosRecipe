// MinionInstanceManager
/** Purpose: Save and Load minions between sessions
    Each minion is keyed by its MinionUUID and keeps its class, level, attributes and the UUIDs of its equipped items
*/

#include "MinionInstanceManager.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

namespace
{
    const FString SavedMinionsFileName = TEXT("SavedMinions.json");
}

UMinionInstanceManager::UMinionInstanceManager()
{
    LoadSavedMinionsFromDisk();
}

void UMinionInstanceManager::SaveMinion(const FMinionSaveData& MinionData)
{
    if (MinionData.MinionUUID.IsEmpty())
    {
        return;
    }

    SavedMinionsByUUID.Add(MinionData.MinionUUID, MinionData);
    WriteSavedMinionsToDisk();
}

void UMinionInstanceManager::LoadSavedMinionsFromDisk()
{
    const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedMinionsFileName;

    FString InputString;
    if (!FFileHelper::LoadFileToString(InputString, *SaveFilePath))
    {
        return;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* MinionsArray;
    if (!RootObject->TryGetArrayField(TEXT("Minions"), MinionsArray))
    {
        return;
    }

    for (const TSharedPtr<FJsonValue>& MinionValue : *MinionsArray)
    {
        const TSharedPtr<FJsonObject>* MinionObject;
        if (!MinionValue->TryGetObject(MinionObject))
        {
            continue;
        }

        FMinionSaveData MinionData;
        if (!FJsonObjectConverter::JsonObjectToUStruct((*MinionObject).ToSharedRef(), &MinionData) || MinionData.MinionUUID.IsEmpty())
        {
            continue;
        }

        SavedMinionsByUUID.Add(MinionData.MinionUUID, MinionData);
    }
}

void UMinionInstanceManager::WriteSavedMinionsToDisk() const
{
    TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> MinionsArray;
    for (const TPair<FString, FMinionSaveData>& Pair : SavedMinionsByUUID)
    {
        TSharedRef<FJsonObject> MinionObject = MakeShared<FJsonObject>();
        FJsonObjectConverter::UStructToJsonObject(FMinionSaveData::StaticStruct(), &Pair.Value, MinionObject, 0, 0);
        MinionsArray.Add(MakeShared<FJsonValueObject>(MinionObject));
    }
    RootObject->SetArrayField(TEXT("Minions"), MinionsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject, Writer);

    const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedMinionsFileName;
    FFileHelper::SaveStringToFile(OutputString, *SaveFilePath);
}

bool UMinionInstanceManager::GetSavedMinion(const FString& MinionUUID, FMinionSaveData& OutMinionData) const
{
    const FMinionSaveData* FoundMinion = SavedMinionsByUUID.Find(MinionUUID);
    if (!FoundMinion)
    {
        return false;
    }

    OutMinionData = *FoundMinion;
    return true;
}

bool UMinionInstanceManager::HasSavedMinion(const FString& MinionUUID) const
{
    return SavedMinionsByUUID.Contains(MinionUUID);
}

void UMinionInstanceManager::RemoveSavedMinion(const FString& MinionUUID)
{
    if (SavedMinionsByUUID.Remove(MinionUUID) > 0)
    {
        WriteSavedMinionsToDisk();
    }
}

void UMinionInstanceManager::ClearAllSavedMinions()
{
    SavedMinionsByUUID.Empty();

    const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedMinionsFileName;
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (PlatformFile.FileExists(*SaveFilePath))
    {
        PlatformFile.DeleteFile(*SaveFilePath);
    }
}

FString UMinionInstanceManager::FindMinionForItem(const FString& ItemUUID) const
{
    for (const TPair<FString, FMinionSaveData>& Pair : SavedMinionsByUUID)
    {
        if (Pair.Value.EquippedItemUUIDs.Contains(ItemUUID))
        {
            return Pair.Key;
        }
    }
    return FString();
}

bool UMinionInstanceManager::RemoveEquippedItem(const FString& MinionUUID, const FString& ItemUUID)
{
    FMinionSaveData* FoundMinion = SavedMinionsByUUID.Find(MinionUUID);
    if (!FoundMinion || FoundMinion->EquippedItemUUIDs.Remove(ItemUUID) == 0)
    {
        return false;
    }

    WriteSavedMinionsToDisk();
    return true;
}
