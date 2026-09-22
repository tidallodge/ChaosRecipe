// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CurrencyStruct.h"

// Saves/loads the player's gold and currency stack counts to/from SavedCurrency.json in the project's
// Saved directory, and resolves currency rows from Currency_DT.
class CHAOSRECIPE_API UCurrencyManager
{
public:
    void SaveCurrency(int32 PlayerGoldCount) const;
    bool LoadPlayerGoldCount(int32& OutPlayerGoldCount) const;

    // Persists the player's currency stack counts (CurrencyId -> count), preserving whatever
    // PlayerGoldCount is already on disk.
    void SaveCurrencyStacks(const TMap<FString, int32>& CurrencyStacks) const;
    bool LoadCurrencyStacks(TMap<FString, int32>& OutCurrencyStacks) const;

    // Deletes SavedCurrency.json from disk (e.g. for a game reset).
    void DeleteSavedCurrency() const;

    bool ValidateCurrencyUpdate(int32& InAdjustValue, int32& InCurrentCurrency) const;

    // Looks up a single currency row from Currency_DT by CurrencyId (case-insensitive).
    bool LoadCurrencyDataRow(const FString& CurrencyId, FCurrencyStruct& OutCurrency) const;

    // Loads every row from Currency_DT.
    bool LoadAllCurrencyRows(TArray<FCurrencyStruct>& OutCurrencies) const;
};
