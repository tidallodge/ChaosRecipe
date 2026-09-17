// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// Saves/loads the player's gold to/from SavedCurrency.json in the project's Saved directory.
// The file holds a single PlayerGoldCount field; there is no per-item or per-session history.
class CHAOSRECIPE_API UCurrencyManager
{
public:
    void SaveCurrency(int32 PlayerGoldCount) const;
    bool LoadPlayerGoldCount(int32& OutPlayerGoldCount) const;
};
