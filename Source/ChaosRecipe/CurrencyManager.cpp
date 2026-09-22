// Fill out your copyright notice in the Description page of Project Settings.

#include "CurrencyManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Engine/DataTable.h"

namespace
{
    const FString SavedCurrencyFileName = TEXT("SavedCurrency.json");

    TSharedPtr<FJsonObject> LoadSavedCurrencyRoot()
    {
        const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedCurrencyFileName;

        FString InputString;
        if (!FFileHelper::LoadFileToString(InputString, *SaveFilePath))
        {
            return nullptr;
        }

        TSharedPtr<FJsonObject> RootObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);
        if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
        {
            return nullptr;
        }

        return RootObject;
    }

    void SaveCurrencyRoot(const TSharedRef<FJsonObject>& RootObject)
    {
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(RootObject, Writer);

        const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedCurrencyFileName;
        FFileHelper::SaveStringToFile(OutputString, *SaveFilePath);
    }
}

void UCurrencyManager::SaveCurrency(int32 PlayerGoldCount) const
{
    // Reads whatever is already on disk first so this gold-only save doesn't clobber CurrencyStacks.
    const TSharedPtr<FJsonObject> ExistingRoot = LoadSavedCurrencyRoot();
    const TSharedRef<FJsonObject> RootObject = ExistingRoot.IsValid() ? ExistingRoot.ToSharedRef() : MakeShared<FJsonObject>();

    RootObject->SetNumberField(TEXT("PlayerGoldCount"), PlayerGoldCount);

    SaveCurrencyRoot(RootObject);
}

bool UCurrencyManager::LoadPlayerGoldCount(int32& OutPlayerGoldCount) const
{
    const TSharedPtr<FJsonObject> RootObject = LoadSavedCurrencyRoot();
    if (!RootObject.IsValid())
    {
        return false;
    }

    double GoldCountValue = 0.0;
    if (!RootObject->TryGetNumberField(TEXT("PlayerGoldCount"), GoldCountValue))
    {
        return false;
    }

    OutPlayerGoldCount = static_cast<int32>(GoldCountValue);
    return true;
}

void UCurrencyManager::SaveCurrencyStacks(const TMap<FString, int32>& CurrencyStacks) const
{
    // Reads whatever is already on disk first so this doesn't clobber PlayerGoldCount.
    const TSharedPtr<FJsonObject> ExistingRoot = LoadSavedCurrencyRoot();
    const TSharedRef<FJsonObject> RootObject = ExistingRoot.IsValid() ? ExistingRoot.ToSharedRef() : MakeShared<FJsonObject>();

    const TSharedRef<FJsonObject> StacksObject = MakeShared<FJsonObject>();
    for (const TPair<FString, int32>& Entry : CurrencyStacks)
    {
        StacksObject->SetNumberField(Entry.Key, Entry.Value);
    }
    RootObject->SetObjectField(TEXT("CurrencyStacks"), StacksObject);

    SaveCurrencyRoot(RootObject);
}

bool UCurrencyManager::LoadCurrencyStacks(TMap<FString, int32>& OutCurrencyStacks) const
{
    OutCurrencyStacks.Reset();

    const TSharedPtr<FJsonObject> RootObject = LoadSavedCurrencyRoot();
    if (!RootObject.IsValid())
    {
        return false;
    }

    const TSharedPtr<FJsonObject>* StacksObject = nullptr;
    if (!RootObject->TryGetObjectField(TEXT("CurrencyStacks"), StacksObject) || !StacksObject || !StacksObject->IsValid())
    {
        return false;
    }

    for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : (*StacksObject)->Values)
    {
        OutCurrencyStacks.Add(Entry.Key, static_cast<int32>(Entry.Value->AsNumber()));
    }

    return true;
}

void UCurrencyManager::DeleteSavedCurrency() const
{
    const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedCurrencyFileName;
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (PlatformFile.FileExists(*SaveFilePath))
    {
        PlatformFile.DeleteFile(*SaveFilePath);
    }
}

bool UCurrencyManager::ValidateCurrencyUpdate(int32& InAdjustValue, int32& InCurrentCurrency) const
{
    if (InAdjustValue <= InCurrentCurrency)
    {
        return true;
    }

    return false;
}

bool UCurrencyManager::LoadCurrencyDataRow(const FString& CurrencyId, FCurrencyStruct& OutCurrency) const
{
    const FString DataTablePath = TEXT("/Game/ItemData/Currency_DT.Currency_DT");
    UDataTable* CurrencyDataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!CurrencyDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("CurrencyManager: Failed to load data table at %s."), *DataTablePath);
        return false;
    }

    for (const FName& RowName : CurrencyDataTable->GetRowNames())
    {
        if (FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CurrencyManager::LoadCurrencyDataRow"), true))
        {
            if (CurrencyRow->CurrencyId.ToString().Equals(CurrencyId, ESearchCase::IgnoreCase))
            {
                OutCurrency = *CurrencyRow;
                return true;
            }
        }
    }

    return false;
}

bool UCurrencyManager::LoadAllCurrencyRows(TArray<FCurrencyStruct>& OutCurrencies) const
{
    OutCurrencies.Reset();

    const FString DataTablePath = TEXT("/Game/ItemData/Currency_DT.Currency_DT");
    UDataTable* CurrencyDataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!CurrencyDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("CurrencyManager: Failed to load data table at %s."), *DataTablePath);
        return false;
    }

    for (const FName& RowName : CurrencyDataTable->GetRowNames())
    {
        if (FCurrencyStruct* CurrencyRow = CurrencyDataTable->FindRow<FCurrencyStruct>(RowName, TEXT("CurrencyManager::LoadAllCurrencyRows"), true))
        {
            OutCurrencies.Add(*CurrencyRow);
        }
    }

    return OutCurrencies.Num() > 0;
}
