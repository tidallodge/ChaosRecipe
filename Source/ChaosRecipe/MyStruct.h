// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MyStruct.generated.h"

USTRUCT(BlueprintType)
struct FMyStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyStruct")
	int32 One = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyStruct")
	int32 Two = 0;
};
