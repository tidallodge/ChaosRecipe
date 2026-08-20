#pragma once

#include "CoreMinimal.h"
#include "TagsStruct.generated.h"

USTRUCT(BlueprintType)
struct FTagsStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Tags")
    bool Etched = false; // locked modifierFTagsStruct

};
