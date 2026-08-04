// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BaseItemStruct.generated.h"

UENUM(BlueprintType)
enum class EItemClass : uint8
{
	Armor UMETA(DisplayName = "Armor"),
	Weapon UMETA(DisplayName = "Weapon"),
	Shield UMETA(DisplayName = "Shield"),
	Jewelry UMETA(DisplayName = "Jewelry"),
	Misc UMETA(DisplayName = "Misc")
};

UENUM(BlueprintType)
enum class EItemSlot : uint8
{
	Chest UMETA(DisplayName = "Chest"),
	Head UMETA(DisplayName = "Head"),
	Gloves UMETA(DisplayName = "Gloves "),
	Boots UMETA(DisplayName = "Boots"),
	HandMain UMETA(DisplayName = "HandMain"),
	HandOff UMETA(DisplayName = "HandOff"),
	HandBoth UMETA(DisplayName = "HandBoth"),
	Neck UMETA(DisplayName = "Neck"),
	Ring UMETA(DisplayName = "Ring"),
	Belt UMETA(DisplayName = "Belt"),
	Misc UMETA(DisplayName = "Misc"),
};

USTRUCT(BlueprintType)
struct FItemAssetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UTexture2D* ItemIcon;

	UPROPERTY(EditAnywhere)
	UStaticMesh* ItemStaticMesh;
};

USTRUCT(BlueprintType)
struct FBaseItemStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemClass ItemClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemSlot ItemSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FItemAssetData ItemAssetData;
};
