// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BaseItemStruct.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FBaseItem : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Item")
	FText ItemId;

	UPROPERTY(EditAnywhere, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, Category = "Item")
	FText ItemBaseType;

	UPROPERTY(EditAnywhere, Category = "Item")
	FText ItemSlot;

	UPROPERTY(EditAnywhere, Category = "Item")
	FText ItemClass;	
};


// Fill out your copyright notice in the Description page of Project Settings.

// #pragma once

// #include "CoreMinimal.h"
// #include "Engine/DataTable.h"
// #include "BaseItemStruct.generated.h"

// USTRUCT(BlueprintType)
// struct FItemAffixes
// {
// 	GENERATED_BODY()

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Affixes")
// 	int32 MaxImplicitMods;
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Affixes")
// 	int32 MaxPrefixMods;
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Affixes")
// 	int32 MaxSuffixMods;
// };

// USTRUCT(BlueprintType)
// struct FItemTags
// {
// 	GENERATED_BODY()

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Tags")
// 	bool HasTags;
// };

// USTRUCT(BlueprintType)
// struct FBaseItemStruct
// {
// 	GENERATED_BODY()

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
// 	FText ItemId;

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
// 	FText ItemName;

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
// 	FText ItemBaseType;

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
// 	FText ItemSlot;

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
// 	FText ItemClass;

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
// 	FItemTags ItemTags;

// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
// 	FItemAffixes ItemAffixes;
// };
