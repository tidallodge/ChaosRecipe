// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseItemStruct.h"
#include "ItemHandler.generated.h"

class UCoreMenu;

UCLASS()
class CHAOSRECIPE_API UItemHandler : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void BindToCoreMenuEvents(UCoreMenu* CoreMenu);

    UFUNCTION()
    void OnItemInfoClicked(FString ItemId);

protected:
    bool LoadItemDataRow(const FString& ItemId, FBaseItemStruct& OutItemData) const;
    void LogItemData(const FBaseItemStruct& ItemData) const;
};
