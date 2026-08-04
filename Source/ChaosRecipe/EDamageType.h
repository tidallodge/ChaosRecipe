#pragma once

#include "CoreMinimal.h"
#include "EDamageType.generated.h"

UENUM(BlueprintType)
enum class EDamageType : uint8
{
    Physical UMETA(DisplayName = "Physical"),
    Fire UMETA(DisplayName = "Fire"),
    Ice UMETA(DisplayName = "Ice"),
    Electric UMETA(DisplayName = "Electric"),
    Poison UMETA(DisplayName = "Poison"),
};
