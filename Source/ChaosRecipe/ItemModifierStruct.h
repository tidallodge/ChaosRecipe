#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BaseItemStruct.h"
#include "BaseWeaponStruct.h"
#include "BaseArmorStruct.h"
#include "ItemModifierStruct.generated.h"

UENUM(BlueprintType)
enum class EAffixType : uint8
{
    Implicit UMETA(DisplayName = "Implicit"),
    Prefix UMETA(DisplayName = "Prefix"),
    Suffix UMETA(DisplayName = "Suffix"),
};

UENUM(BlueprintType)
enum class EModifierOperator : uint8
{
    Addition UMETA(DisplayName = "Addition"),
    Multiplication UMETA(DisplayName = "Multiplication"),
    GlobalMod UMETA(DisplayName = "GlobalMod"),
    UniqueModifier UMETA(DisplayName = "UniqueModifier"),
};

USTRUCT(BlueprintType)
struct FValidItemClasses
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Classes")
    bool Armor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Classes")
    bool Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Classes")
    bool Shield;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Classes")
    bool Jewelry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Classes")
    bool Misc;
    
};

USTRUCT(BlueprintType)
struct FValidWeaponTypes
{
    GENERATED_BODY()

    // Mirrors EWeaponType (BaseWeaponStruct.h)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Weapon Types")
    bool Sword;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Weapon Types")
    bool Axe;
};

USTRUCT(BlueprintType)
struct FValidArmorTypes
{
    GENERATED_BODY()

    // Mirrors EArmorType (BaseArmorStruct.h)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Armor Types")
    bool Head;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Armor Types")
    bool Chest;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Armor Types")
    bool Gloves;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Armor Types")
    bool Boots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Armor Types")
    bool Shield;
};

USTRUCT(BlueprintType)
struct FValidShieldTypes
{
    GENERATED_BODY()

    // No dedicated EShieldType exists yet; derived from the relevant slot in EItemSlot (BaseItemStruct.h)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Shield Types")
    bool HandOff;
};

USTRUCT(BlueprintType)
struct FValidJewelryTypes
{
    GENERATED_BODY()

    // No dedicated EJewelryType exists yet; derived from the relevant slots in EItemSlot (BaseItemStruct.h)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Jewelry Types")
    bool Neck;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Jewelry Types")
    bool Ring;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Jewelry Types")
    bool Belt;
};

USTRUCT(BlueprintType)
struct FValidMiscTypes
{
    GENERATED_BODY()

    // No dedicated EMiscType exists yet; derived from the relevant slot in EItemSlot (BaseItemStruct.h)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Misc Types")
    bool Misc;
};

USTRUCT(BlueprintType)
struct FValidItemTypesOverride
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Types")
    FValidArmorTypes Armor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Types")
    FValidWeaponTypes Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Types")
    FValidShieldTypes Shield;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Types")
    FValidJewelryTypes Jewelry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valid Item Types")
    FValidMiscTypes Misc;

};

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
    bool Poison;

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

};

USTRUCT(BlueprintType)
struct FItemModifierDamageStruct
{
    GENERATED_BODY()

    // Weapon Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Modifier")
    float PhysicalDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Modifier")
    float FireDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Modifier")
    float IceDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Modifier")
    float ElectricDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Modifier")
    float PoisonDamage = 0.f;
};

USTRUCT(BlueprintType)
struct FItemModifierMitigationStruct
{
    GENERATED_BODY()

    // Armor Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Modifier")
    float PhysicalReduction = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Modifier")
    float Evade = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Modifier")
    float Overshield = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Modifier")
    float Block = 0.f;
};

USTRUCT(BlueprintType)
struct FItemModifierReistanceStruct
{
    GENERATED_BODY()

    // Resistance Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance Modifier")
    float FireResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance Modifier")
    float IceResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance Modifier")
    float ElectricResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistance Modifier")
    float PoisonResistance = 0.f;
};

USTRUCT(BlueprintType)
struct FItemModifierAffectedAttributes
{
    GENERATED_BODY()
    /**  Struct to reference for the magnitude of modifying each possible modifier.
    Defaults are 0 for no change for most attributes, 1 for modifying by value chosen in MinMaxRange.
    Any non-zero float will mutliply the chosen MinMaxRange value and modify the set attribute by the result */
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modified Stats")
    float WeaponAttackRate = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modified Stats")
    float WeaponCriticalChance = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modified Stats")
    float WeaponCriticalMulti = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modified Stats")
    FItemModifierDamageStruct DamageModifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modified Stats")
    FItemModifierMitigationStruct MigitationModifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modified Stats")
    FItemModifierReistanceStruct ResistanceModifier;
};

USTRUCT(BlueprintType)
struct FItemModifierStruct : public FTableRowBase
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FText ModifierId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FText ModifierName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FTagsStruct ItemTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FModifierTags ModifierTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    int32 ModifierWeight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    int32 ModifierTier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    int32 ModifierRequiredLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    EAffixType ModifierAffixType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    TArray<int32> MinMaxRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FItemModifierAffectedAttributes ModifiedAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    EModifierOperator ModifierOperator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FText UniqueModifierId; // set this for mods that add unique effects, not modifying a current stat on an item

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FValidItemClasses ValidItemClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Modifiers")
    FValidItemTypesOverride ValidItemTypesOverride;

};