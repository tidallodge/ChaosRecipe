// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "MinionClassStruct.h"
#include "CoreGameMode.generated.h"

class UCoreMenu;
class UStoreManager;
class UItemHandler;
class UPlayerInventory;
class UPlayerMinion;
class UMinionHandler;
class UHireShopHandler;
class UBattleManager;
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
	TSubclassOf<UUserWidget> MinionButtonClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	TSubclassOf<UUserWidget> SingleButtonClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* BaseItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* BaseWeaponDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* BaseArmorDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* ItemModifierDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* MinionClassDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* EnemyDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hard References")
	UDataTable* ZoneDataTable;

	// The MinionClass_DT class a new game's first minion is created with (when SavedMinions.json has no
	// minions). It's selected in the CoreMenu's minion HP bar.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EMinionClass StartingMinionClass = EMinionClass::Warrior;

	UPROPERTY()
	UItemHandler* ItemHandler;

	// Rooted here so it isn't garbage-collected while only referenced via delegate bindings; keeps
	// ItemHandler's sell flow and this the same PlayerInventory instance for the whole session.
	UPROPERTY()
	UPlayerInventory* PlayerInventory;

	// Creates, loads and saves every minion by its MinionUUID.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	UMinionHandler* MinionHandler;

	// Keeps (and saves) the minions the hire shop offers, and rerolls them for gold.
	UPROPERTY()
	UHireShopHandler* HireShopHandler;

	// The first minion in the active party when the game started, selected in the CoreMenu's HP bar.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	UPlayerMinion* Minion;

	// Runs MinionHandler's active party through every stage of the zone picked on CoreMenu's map, each time its
	// BattlePlayButton is clicked, spawning each stage's enemies as it goes.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	UBattleManager* BattleManager;
};
