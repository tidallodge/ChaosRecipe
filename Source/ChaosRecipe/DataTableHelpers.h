#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

// The same linear row search ItemHandler and CurrencyManager each do for their own tables, shared by the
// classes that read several different tables (UPlayerMinion, UEnemy).
namespace DataTableHelpers
{
    // Copies out the first row of the data table at DataTablePath that matches Predicate.
    template <typename TRow, typename TPredicate>
    bool FindRow(const TCHAR* DataTablePath, TPredicate Predicate, TRow& OutRow)
    {
        UDataTable* DataTable = LoadObject<UDataTable>(nullptr, DataTablePath);
        if (!DataTable)
        {
            UE_LOG(LogTemp, Error, TEXT("DataTableHelpers: Failed to load data table at %s."), DataTablePath);
            return false;
        }

        for (const FName& RowName : DataTable->GetRowNames())
        {
            if (const TRow* Row = DataTable->FindRow<TRow>(RowName, TEXT("DataTableHelpers::FindRow"), true))
            {
                if (Predicate(*Row))
                {
                    OutRow = *Row;
                    return true;
                }
            }
        }

        return false;
    }

    // Copies out every row of the data table at DataTablePath.
    template <typename TRow>
    bool LoadAllRows(const TCHAR* DataTablePath, TArray<TRow>& OutRows)
    {
        OutRows.Reset();

        UDataTable* DataTable = LoadObject<UDataTable>(nullptr, DataTablePath);
        if (!DataTable)
        {
            UE_LOG(LogTemp, Error, TEXT("DataTableHelpers: Failed to load data table at %s."), DataTablePath);
            return false;
        }

        for (const FName& RowName : DataTable->GetRowNames())
        {
            if (const TRow* Row = DataTable->FindRow<TRow>(RowName, TEXT("DataTableHelpers::LoadAllRows"), true))
            {
                OutRows.Add(*Row);
            }
        }

        return OutRows.Num() > 0;
    }
}
