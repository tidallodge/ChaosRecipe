
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ModifierTagsStruct.generated.h"


USTRUCT(BlueprintType)
struct FModifierTags
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Physical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Fire;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Ice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Electric;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Abyssal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Damage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Attack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Defense;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Resistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Speed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Critical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Life;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Armor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Evade;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Tags")
    bool Overshield;
};