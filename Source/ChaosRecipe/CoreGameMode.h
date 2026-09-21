// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "CoreGameMode.generated.h"

class UCoreMenu;
class UStoreManager;
class UItemHandler;
class UPlayerInventory;
class UDataTable;

/**
 *
 */
UCLASS()
class CHAOSRECIPE_API ACoreGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoreGameMode();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UCoreMenu> CoreMenuClass;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	UCoreMenu* CoreMenuWidget;

	// Hard references to assets that are otherwise only ever loaded by string path at runtime
	// (CoreMenu::PopulateShopGrid/PopulatePlayerStash, ItemHandler::LoadItemDataRow and friends).
	// Nothing else in the project references them, so without a hard reference here the cooker
	// can exclude them from a packaged build entirely, making those LoadObject/LoadClass calls
	// silently return nullptr outside the editor even though they work fine in PIE. This class's
	// CDO is guaranteed to be cooked because GMB_CoreGameMode (its BP subclass) is the project's
	// GlobalDefaultGameMode, so any hard reference held here is guaranteed to be cooked with it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	TSubclassOf<UUserWidget> SingleImageButtonClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* BaseItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* BaseWeaponDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* BaseArmorDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* ItemModifierDataTable;

	UPROPERTY()
	UItemHandler* ItemHandler;

	// Rooted here so it isn't garbage-collected while only referenced via delegate bindings; keeps
	// ItemHandler's sell flow and this the same PlayerInventory instance for the whole session.
	UPROPERTY()
	UPlayerInventory* PlayerInventory;
};
