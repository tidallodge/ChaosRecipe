#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EDamageType.h"
#include "BaseWeaponStruct.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Sword UMETA(DisplayName = "Sword"),
    Axe UMETA(DisplayName = "Axe"),
};

UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
    MainHand UMETA(DisplayName = "MainHand"),
    OffHand UMETA(DisplayName = "OffHand"),
    TwoHand UMETA(DisplayName = "TwoHand"),
};

USTRUCT(BlueprintType)
struct FWeaponBaseDamage 
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FIntPoint BasePhysicalDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint BaseFireDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint BaseIceDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint BaseElectricDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint BasePoisonDamage;
};

USTRUCT(BlueprintType)
struct FWeaponLocalDamage 
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FIntPoint LocalPhysicalDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint LocalFireDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint LocalIceDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint LocalElectricDamage;

    UPROPERTY(EditAnywhere)
    FIntPoint LocalPoisonDamage;
};

USTRUCT(BlueprintType)
struct FBaseWeaponStruct : public FTableRowBase
{
    GENERATED_BODY()

   	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EWeaponType WeaponType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EWeaponSlot WeaponSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FWeaponBaseDamage WeaponBaseDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FWeaponLocalDamage WeaponLocalDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float WeaponBaseAttackRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float WeaponBaseCritChance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float WeaponBaseCritMulti;
};