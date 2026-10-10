// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinionClassStruct.h"
#include "MinionInstanceManager.h"
#include "MinionHandler.generated.h"

class UCoreMenu;
class UItemHandler;
class UPlayerMinion;

UCLASS()
class CHAOSRECIPE_API UMinionHandler : public UObject
{
	GENERATED_BODY()

public:
	// Also keeps CoreMenu's minion buttons in step with the minions created or loaded from then on.
	UFUNCTION()
	void BindToCoreMenuEvents(UCoreMenu* CoreMenu);

	// Binds every minion, already loaded or loaded later, to ItemHandler's events, so a sold item is
	// unequipped and a rerolled one is re-read.
	UFUNCTION()
	void BindToItemHandlerEvents(UItemHandler* ItemHandler);

	// Creates a brand-new minion of InMinionClass at InLevel with a new MinionUUID, and saves it. Returns null
	// if the class has no MinionClass_DT row.
	UFUNCTION(BlueprintCallable, Category = "Minion")
	UPlayerMinion* CreateMinion(EMinionClass InMinionClass, int32 InLevel = 1);

	// Rebuilds the minion saved under MinionUUID, re-equipping its saved items from the stash. Returns the
	// minion as-is if it's already loaded, or null if there's no such saved minion.
	UFUNCTION(BlueprintCallable, Category = "Minion")
	UPlayerMinion* LoadMinion(const FString& MinionUUID);

	// Loads every minion in SavedMinions.json.
	UFUNCTION(BlueprintCallable, Category = "Minion")
	TArray<UPlayerMinion*> LoadSavedMinions();

	UFUNCTION(BlueprintPure, Category = "Minion")
	UPlayerMinion* FindMinion(const FString& MinionUUID) const;

	// Every minion created or loaded this session.
	UFUNCTION(BlueprintPure, Category = "Minion")
	TArray<UPlayerMinion*> GetMinions() const;

	// Equips a stash item on the minion with MinionUUID, then takes it off whichever other minion had it, since
	// an item can only be equipped by one minion at a time.
	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	bool EquipItemOnMinion(const FString& MinionUUID, const FString& ItemUUID);

	// Also works on a saved minion that isn't loaded, by editing its save directly.
	UFUNCTION(BlueprintCallable, Category = "Minion|Equipment")
	bool UnequipItemFromMinion(const FString& MinionUUID, const FString& ItemUUID);

	// The MinionUUID of the minion that has ItemUUID equipped, or empty if none has it.
	UFUNCTION(BlueprintPure, Category = "Minion|Equipment")
	FString FindMinionForItem(const FString& ItemUUID) const;

	UFUNCTION(BlueprintCallable, Category = "Minion")
	bool SetMinionLevel(const FString& MinionUUID, int32 NewLevel);

	// Bound to CoreMenu's OnResetGameButtonClickedEvent: wipes SavedMinions.json, then puts every loaded minion
	// back to level 1 of its class with nothing equipped (the stash is wiped too) and saves it again under the
	// same MinionUUID.
	UFUNCTION()
	void OnResetGame();

protected:
	// Bound to every minion's OnSaveDataChangedEvent, so any change to its level, attributes or equipment is saved.
	UFUNCTION()
	void HandleMinionSaveDataChanged(FString MinionUUID);

	// Tracks a newly created or loaded minion under its MinionUUID, binds it to item events, and saves it.
	void RegisterMinion(UPlayerMinion* Minion);

	void SaveMinion(const UPlayerMinion* Minion);

	// Pushes every loaded minion to the bound CoreMenu's minion buttons, if a CoreMenu has been bound.
	void UpdateMinionButtons() const;

	// The only instance that writes SavedMinions.json, so saving one minion can't clobber another's save.
	UMinionInstanceManager SavedMinionsManager;

	UPROPERTY()
	TMap<FString, TObjectPtr<UPlayerMinion>> MinionsByUUID;

	UPROPERTY()
	TObjectPtr<UItemHandler> BoundItemHandler = nullptr;

	UPROPERTY()
	TObjectPtr<UCoreMenu> BoundCoreMenu = nullptr;
};
