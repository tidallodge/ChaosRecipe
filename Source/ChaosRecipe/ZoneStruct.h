#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyStruct.h"
#include "ZoneStruct.generated.h"

// Zone_DT, the data table of FZoneStruct rows.
inline constexpr const TCHAR* ZoneDataTablePath = TEXT("/Game/ZoneData/Zone_DT.Zone_DT");

// One enemy a zone stage can spawn. Only its Enemy_DT EnemyId is kept here: everything about the enemy itself comes
// from its Enemy_DT row when it's spawned.
USTRUCT(BlueprintType)
struct FZoneEnemySpawn
{
    GENERATED_BODY()

    // An Enemy_DT EnemyId (picked from a dropdown in the data table editor).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (GetOptions = "ChaosRecipe.Enemy.GetEnemyIdOptions"))
    FString EnemyId;

    // How likely each open spot in a group is to be this enemy, relative to the stage's other enemies (e.g. 3 is
    // three times as likely as 1). At 0 it only spawns through MinPerGroup.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0"))
    float SpawnWeight = 1.f;

    // Every group has at least this many of this enemy (e.g. 1 for a stage's boss), even if that takes the group past
    // the stage's GroupSize.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0"))
    int32 MinPerGroup = 0;

    // No group has more than this many of this enemy. 0 means no limit.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0"))
    int32 MaxPerGroup = 0;
};

// A zone stage's drop table: what the stage itself drops once it's cleared, on top of whatever its enemies dropped.
// Rolled the same way as an enemy's drops (UEnemy::RollDrops). Left empty (the default), the stage drops nothing.
USTRUCT(BlueprintType)
struct FZoneStageDrops
{
    GENERATED_BODY()

    // Gold dropped, rolled between X and Y.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    FIntPoint GoldDrop = FIntPoint::ZeroValue;

    // Every entry rolls independently.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops", meta = (TitleProperty = "CurrencyId"))
    TArray<FEnemyCurrencyDrop> CurrencyDrops;
};

// One of a zone's stages: which enemies its groups can be made of, how many of them, and what clearing it drops.
USTRUCT(BlueprintType)
struct FZoneStage
{
    GENERATED_BODY()

    // How many enemies are in a group, rolled between X and Y each time one spawns.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
    FIntPoint GroupSize = FIntPoint(1, 1);

    // The enemy types valid for this stage, each with its own count limits.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (TitleProperty = "EnemyId"))
    TArray<FZoneEnemySpawn> Enemies;

    // Rolled and granted once this stage's enemies are all defeated, before the battle moves on to the next stage (or
    // ends as won, after the last one).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
    FZoneStageDrops StageEndDrops;
};

// One Zone_DT row: a place to battle, made of a set number of stages the player works through in order, each spawning
// its own enemy groups.
USTRUCT(BlueprintType)
struct FZoneStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
    FText ZoneId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
    FText ZoneName;

    // The zone's stages, in the order they're played: the zone has exactly this many, and a stage's StageIndex is its
    // index here (the first is 0).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
    TArray<FZoneStage> Stages;

    int32 GetStageCount() const { return Stages.Num(); }

    // The stage at StageIndex, or null past the zone's last stage.
    const FZoneStage* GetStage(int32 StageIndex) const;

    // Rolls a new enemy group for the stage at StageIndex: a size from its GroupSize, each enemy's MinPerGroup, then
    // the rest picked by SpawnWeight without taking anyone past their MaxPerGroup. Returns the group's Enemy_DT
    // EnemyIds (an enemy appears once per copy in the group), or none if the zone has no such stage.
    TArray<FString> RollEnemyGroup(int32 StageIndex) const;

    // Rolls the StageEndDrops of the stage at StageIndex. Returns empty loot if it has none configured (or none of its
    // entries hit), or if the zone has no such stage.
    FEnemyLoot RollStageEndDrops(int32 StageIndex) const;

    // Removes every stage's StageEndDrops currency whose CurrencyId isn't in Currency_DT, the same way UEnemy does for
    // its own drops. Call once after loading the row, before rolling any of them.
    void RemoveUnknownCurrencyDrops();
};
