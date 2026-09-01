#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ModifierBucketsStruct.generated.h"


USTRUCT(BlueprintType)
struct FWeaponModifierBuckets
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalFlatPhysDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalPercentPhysDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalFlatFireDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalFlatColdDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalFlatElectricDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalFlatPoisonDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalAttackRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalCritChance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool LocalCritDamage;
};

USTRUCT(BlueprintType)
struct FArmorModifierBuckets
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool PhysicalMitigation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool Evade;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool OverShield;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool life;
};

USTRUCT(BlueprintType)
struct FJewelryModifierBuckets
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool FireResist;
};

USTRUCT(BlueprintType)
struct FMiscModifierBuckets
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool Life;
};

USTRUCT(BlueprintType)
struct FGlobalModifierBuckets
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool FireResist;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool ColdResist;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool ElectricResist;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool PoisonResist;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool GlobalAllSkills;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    bool UniqueSkill;
};

USTRUCT(BlueprintType)
struct FModifierBuckets
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    FWeaponModifierBuckets WeaponModifierBuckets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    FArmorModifierBuckets ArmorModifierBuckets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    FJewelryModifierBuckets JewelryModifierBuckets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    FMiscModifierBuckets MiscModifierBuckets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier Buckets")
    FGlobalModifierBuckets GlobalModifierBuckets;
};