// ZoneStruct
/** Purpose: Roll a random enemy group for one of a zone's stages
*/

#include "ZoneStruct.h"
#include "CombatTypes.h"

const FZoneStage* FZoneStruct::GetStage(int32 StageIndex) const
{
    return Stages.IsValidIndex(StageIndex) ? &Stages[StageIndex] : nullptr;
}

TArray<FString> FZoneStruct::RollEnemyGroup(int32 StageIndex) const
{
    TArray<FString> EnemyIds;

    const FZoneStage* Stage = GetStage(StageIndex);
    if (!Stage)
    {
        UE_LOG(LogTemp, Warning, TEXT("Zone: %s has no stage %d (it has %d)."), *ZoneId.ToString(), StageIndex, GetStageCount());
        return EnemyIds;
    }

    const TArray<FZoneEnemySpawn>& Enemies = Stage->Enemies;

    // How many of each enemy (same index as Enemies) the group has so far, starting from every MinPerGroup.
    TArray<int32> Counts;
    Counts.SetNumZeroed(Enemies.Num());
    int32 GroupCount = 0;
    for (int32 Index = 0; Index < Enemies.Num(); ++Index)
    {
        const FZoneEnemySpawn& Spawn = Enemies[Index];
        if (!Spawn.EnemyId.IsEmpty())
        {
            Counts[Index] = Spawn.MaxPerGroup > 0 ? FMath::Min(Spawn.MinPerGroup, Spawn.MaxPerGroup) : Spawn.MinPerGroup;
            GroupCount += Counts[Index];
        }
    }

    auto CanTakeMore = [&Enemies, &Counts](int32 Index)
    {
        const FZoneEnemySpawn& Spawn = Enemies[Index];
        return !Spawn.EnemyId.IsEmpty() && Spawn.SpawnWeight > 0.f && (Spawn.MaxPerGroup <= 0 || Counts[Index] < Spawn.MaxPerGroup);
    };

    const int32 GroupSize = CombatMath::RollRange(Stage->GroupSize);
    while (GroupCount < GroupSize)
    {
        float TotalWeight = 0.f;
        for (int32 Index = 0; Index < Enemies.Num(); ++Index)
        {
            if (CanTakeMore(Index))
            {
                TotalWeight += Enemies[Index].SpawnWeight;
            }
        }

        // Every enemy is at its MaxPerGroup (or can't fill open spots), so the group stays smaller.
        if (TotalWeight <= 0.f)
        {
            break;
        }

        float Pick = FMath::FRand() * TotalWeight;
        int32 Picked = INDEX_NONE;
        for (int32 Index = 0; Index < Enemies.Num(); ++Index)
        {
            if (!CanTakeMore(Index))
            {
                continue;
            }

            // Set before the check, so float rounding at the very end of the range still lands on an enemy.
            Picked = Index;
            Pick -= Enemies[Index].SpawnWeight;
            if (Pick < 0.f)
            {
                break;
            }
        }

        ++Counts[Picked];
        ++GroupCount;
    }

    for (int32 Index = 0; Index < Enemies.Num(); ++Index)
    {
        for (int32 Copy = 0; Copy < Counts[Index]; ++Copy)
        {
            EnemyIds.Add(Enemies[Index].EnemyId);
        }
    }
    return EnemyIds;
}
