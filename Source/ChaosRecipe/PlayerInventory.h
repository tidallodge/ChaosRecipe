// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <unordered_map>
#include "CoreMinimal.h"
#include "PlayerInventory.generated.h"


struct FStringHash
{
    size_t operator()(FString const& Key) const noexcept
    {
        return static_cast<size_t>(GetTypeHash(Key));
    }
};

struct FStringEqual
{
    bool operator()(FString const& A, FString const& B) const noexcept
    {
        return A == B;
    }
};

class UCoreMenu;
class UStoreManager;

UCLASS()
class CHAOSRECIPE_API UPlayerInventory : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void BindToCoreMenuEvents(UCoreMenu* CoreMenu);
	UFUNCTION()
	void BindToStoreManagerEvents(UStoreManager* StoreManager);

	UFUNCTION()
	void HandleStoreSale(FString ItemType, int32 ItemValue);
	UFUNCTION()
	void HandleStoreBuy(FString ItemType, int32 ItemValue);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	float ItemValueModifier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 PlayerMoneyCount;

	UPROPERTY()
	bool ValidSale = 0;
	UPROPERTY()
	bool ValidBuy = 1;

	std::unordered_map<FString, int32, FStringHash, FStringEqual> ItemCountById;

	UCoreMenu* CoreMenuRef;
};
