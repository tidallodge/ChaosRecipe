#pragma once

#include "CoreMinimal.h"
#include "EMitigationType.generated.h"

UENUM(BlueprintType)
enum class EMitigationType : uint8
{
    PhysicalReduction UMETA(DisplayName = "PhysicalReduction"),
    Evade UMETA(DisplayName = "Evade"),
    Overshield UMETA(DisplayName = "Overshield"),
};
