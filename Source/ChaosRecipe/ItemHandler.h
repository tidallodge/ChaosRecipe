// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TagsStruct.h"
#include "BaseItemStruct.h"
#include "BaseWeaponStruct.h"
#include "BaseArmorStruct.h"
#include "ItemHandler.generated.h"

class UCoreMenu;

USTRUCT(BlueprintType)
struct FItemWeaponStatsStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    FText ItemId;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    FText UUID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    int32 ItemLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    FTagsStruct Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, FWeaponBaseDamage> WeaponDamage; // FString for easy look up for what damage type, FWeaponBaseDamage struct for full damage info

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    float AttackRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, int32> ImplicitModifiers; // FString for ModifierId, int32 for modifier range roll (0 through max range for each mod)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, int32> PrefixModifiers; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
    TMap<FString, int32> SuffixModifiers; 

};

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
