// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "CoreGameMode.generated.h"

class UCoreMenu;
class UStoreManager;

/**
 * 
 */
UCLASS()
class CHAOSRECIPE_API ACoreGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UCoreMenu> CoreMenuClass;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	UCoreMenu* CoreMenuWidget;
};
