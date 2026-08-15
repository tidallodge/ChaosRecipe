// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TestStruct.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FTagsStructTest : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Tags")
    bool LockedMod = false;
};

class CHAOSRECIPE_API TestStruct
{
public:
	TestStruct();
	~TestStruct();
	
};
