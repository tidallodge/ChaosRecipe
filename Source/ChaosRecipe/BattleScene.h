// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyStruct.h"
#include "BattleScene.generated.h"

class UEnemy;
class UStaticMeshComponent;

// One enemy shown by ABattleScene, and the mesh standing in for it.
USTRUCT()
struct FBattleSceneEnemy
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UEnemy> Enemy = nullptr;

	// Null if the enemy has no EnemyStaticMesh in Enemy_DT, or there was no slot left for it.
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
};

// Placed in L_MenuScene: shows the current stage's enemy group as their Enemy_DT static meshes, one per slot.
// UBattleManager finds it through CoreGameMode and tells it which enemies to show as each stage starts.
UCLASS()
class CHAOSRECIPE_API ABattleScene : public AActor
{
	GENERATED_BODY()

public:
	ABattleScene();

	// Replaces the enemies shown with Enemies, the Nth one standing at EnemySlots[N] (the same order as their HP bars).
	// Each mesh is hidden once its enemy is defeated.
	void ShowEnemies(const TArray<UEnemy*>& Enemies);

	// Removes every enemy mesh.
	void ClearEnemies();

protected:
	// Bound to every shown enemy's OnKilledEvent: hides the mesh of each shown enemy that's been defeated.
	UFUNCTION()
	void HandleEnemyKilled(FString EnemyId, const FEnemyLoot& Loot);

	// Where each enemy in a group stands, relative to this actor. Drag the diamonds in the viewport to place them;
	// an enemy past the last slot isn't shown.
	UPROPERTY(EditAnywhere, Category = "Battle Scene", meta = (MakeEditWidget = true))
	TArray<FTransform> EnemySlots;

	UPROPERTY()
	TArray<FBattleSceneEnemy> ShownEnemies;
};
