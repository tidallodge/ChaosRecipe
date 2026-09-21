
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ModifierTagsStruct.h"
#include "CurrencyStruct.generated.h"

UENUM(BlueprintType)
enum class EBaseCurrencyType : uint8
{
    Basic UMETA(DisplayName = "Basic Currency"),
    Exotic UMETA(DisplayName = "Exotic Currency"),
};

UENUM(BlueprintType)
enum class EModifierModificationType : uint8
{
    Add UMETA(DisplayName = "Add Modifier"),
    Remove UMETA(DisplayName = "Remove Modifier"),
    RandomizeMods UMETA(DisplayName = "Randomize Modifiers"),
    RandomizeTiers UMETA(DisplayName = "Randomize Tiers"),
    RandomizeValues UMETA(DisplayName = "Randomize Values"),
    Other UMETA(DisplayName = "Other"),
};

USTRUCT(BlueprintType)
struct FExoticCurrencyMethods
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exotic Methods")
    bool Etch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exotic Methods")
    bool RandomizeTiers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exotic Methods")
    bool TempLock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exotic Methods")
    bool AddSpecificModifierTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exotic Methods")
    bool AddSpecialModPool;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exotic Methods")
    bool AddUniqueModifier;
};

USTRUCT(BlueprintType)
struct FAffectedAffixes
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affected Affixes")
    bool Prefixes;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affected Affixes")
    bool Suffixes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affected Affixes")
    bool Implicits;
};

USTRUCT(BlueprintType)
struct FCurrencyStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    FText CurrencyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    FText CurrencyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    EBaseCurrencyType BaseCurrencyType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    EModifierModificationType ModifierModificationType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency") 
    TArray<int32> ModifiersAffectedWeightedRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    FAffectedAffixes AffectedAffixes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    FModifierTags CurrencyModifierTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    FExoticCurrencyMethods ExoticCurrencyMethods;
};
