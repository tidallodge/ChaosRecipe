// BattleScene
/** Purpose: Show the current battle stage's enemies in the level as their Enemy_DT static meshes
    Each enemy in the group gets a mesh at its own slot, hidden once it's defeated
    The meshes are replaced every stage and removed when the battle stops
*/

#include "BattleScene.h"
#include "Components/StaticMeshComponent.h"
#include "Enemy.h"

ABattleScene::ABattleScene()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	EnemySlots = { FTransform(FVector(0.f, -200.f, 0.f)), FTransform(FVector::ZeroVector), FTransform(FVector(0.f, 200.f, 0.f)) };
}

void ABattleScene::ShowEnemies(const TArray<UEnemy*>& Enemies)
{
	ClearEnemies();

	for (int32 Index = 0; Index < Enemies.Num(); ++Index)
	{
		UEnemy* Enemy = Enemies[Index];
		if (!Enemy)
		{
			continue;
		}

		FBattleSceneEnemy& Entry = ShownEnemies.AddDefaulted_GetRef();
		Entry.Enemy = Enemy;
		Enemy->OnKilledEvent.AddDynamic(this, &ABattleScene::HandleEnemyKilled);

		UStaticMesh* StaticMesh = Enemy->GetStaticMesh();
		if (!StaticMesh || !EnemySlots.IsValidIndex(Index))
		{
			UE_LOG(LogTemp, Warning, TEXT("BattleScene: Can't show %s (%s)."), *Enemy->GetEnemyId(),
				StaticMesh ? TEXT("no EnemySlots entry left for it") : TEXT("no EnemyStaticMesh in Enemy_DT"));
			continue;
		}

		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this);
		MeshComponent->SetStaticMesh(StaticMesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetupAttachment(RootComponent);
		MeshComponent->SetRelativeTransform(EnemySlots[Index]);
		MeshComponent->RegisterComponent();
		Entry.Mesh = MeshComponent;
	}
}

void ABattleScene::ClearEnemies()
{
	for (const FBattleSceneEnemy& Entry : ShownEnemies)
	{
		if (Entry.Enemy)
		{
			Entry.Enemy->OnKilledEvent.RemoveDynamic(this, &ABattleScene::HandleEnemyKilled);
		}
		if (Entry.Mesh)
		{
			Entry.Mesh->DestroyComponent();
		}
	}
	ShownEnemies.Reset();
}

void ABattleScene::HandleEnemyKilled(FString EnemyId, const FEnemyLoot& Loot)
{
	// OnKilledEvent only says which EnemyId died, and a group can hold several of the same one, so every defeated
	// enemy's mesh is hidden rather than looking it up by id.
	for (const FBattleSceneEnemy& Entry : ShownEnemies)
	{
		if (Entry.Mesh && Entry.Enemy && Entry.Enemy->IsDefeated())
		{
			Entry.Mesh->SetVisibility(false);
		}
	}
}
