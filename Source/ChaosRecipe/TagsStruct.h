#pragma once

#include "CoreMinimal.h"
#include "TagsStruct.generated.h"

USTRUCT(BlueprintType)
struct FTagsStruct
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Tags")
    bool LockedMod = false;
};
