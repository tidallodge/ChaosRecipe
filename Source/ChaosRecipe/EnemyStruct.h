#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EDamageType.h"
#include "CombatAbilityStruct.h"
#include "EnemyStruct.generated.h"

USTRUCT(BlueprintType)
struct FEnemyCurrencyDrop
{
    GENERATED_BODY()

    // A Currency_DT CurrencyId (picked from a dropdown in the data table editor).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops", meta = (GetOptions = "ChaosRecipe.Enemy.GetCurrencyIdOptions"))
    FString CurrencyId;

    // Percent chance (0-100) that this entry drops at all. Every entry rolls independently.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops", meta = (ClampMin = "0", ClampMax = "100"))
    float DropChance = 100.f;

    // How many drop when it does, rolled between X and Y.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    FIntPoint DropCount = FIntPoint(1, 1);
};

// What an enemy actually dropped when it was killed (already rolled), sent with UEnemy::OnKilledEvent.
USTRUCT(BlueprintType)
struct FEnemyLoot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Drops")
    int32 Gold = 0;

    // CurrencyId -> how many dropped.
    UPROPERTY(BlueprintReadOnly, Category = "Drops")
    TMap<FString, int32> CurrencyCounts;
};

USTRUCT(BlueprintType)
struct FEnemyAssetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UTexture2D* EnemyIcon;

	UPROPERTY(EditAnywhere)
	UStaticMesh* EnemyStaticMesh;
};

USTRUCT(BlueprintType)
struct FEnemyStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    FText EnemyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    FText EnemyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    int32 MaxHealth = 10;

    // Absorbs damage before health does. The enemy starts with all of it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    int32 Overshield = 0;

    // Same scale as armor: mitigation equal to 100 halves physical damage (see CombatMath::ResolveHit).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
    int32 PhysicalMitigation = 0;

    // Percent chance (0-100) to evade attacks. Spells can't be evaded.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (ClampMin = "0", ClampMax = "100"))
    int32 EvadeChance = 0;

    // Percent damage reduction per type; negative values are weaknesses. Physical goes through
    // PhysicalMitigation instead, so a Physical entry here is ignored.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
    TMap<EDamageType, float> Resistances;

    // Min-max damage per type for the enemy's basic attack (UEnemy::Attack).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
    TMap<EDamageType, FIntPoint> Damage;

    // Basic attacks per second.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
    float AttackRate = 1.f;

    // Cast by AbilityId with UEnemy::CastSpell. An Attack-type entry here builds on Damage like a
    // basic attack would, so it also works for special attacks.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
    TArray<FCombatAbilityStruct> Spells;

    // Gold dropped on death, rolled between X and Y.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    FIntPoint GoldDrop = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    TArray<FEnemyCurrencyDrop> CurrencyDrops;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FEnemyAssetData EnemyAssetData;
};
