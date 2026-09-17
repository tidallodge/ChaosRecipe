// Fill out your copyright notice in the Description page of Project Settings.

#include "CurrencyManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FString SavedCurrencyFileName = TEXT("SavedCurrency.json");
}

void UCurrencyManager::SaveCurrency(int32 PlayerGoldCount) const
{
    TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
    RootObject->SetNumberField(TEXT("PlayerGoldCount"), PlayerGoldCount);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject, Writer);

    const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedCurrencyFileName;
    FFileHelper::SaveStringToFile(OutputString, *SaveFilePath);
}

bool UCurrencyManager::LoadPlayerGoldCount(int32& OutPlayerGoldCount) const
{
    const FString SaveFilePath = FPaths::ProjectSavedDir() / SavedCurrencyFileName;

    FString InputString;
    if (!FFileHelper::LoadFileToString(InputString, *SaveFilePath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
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
