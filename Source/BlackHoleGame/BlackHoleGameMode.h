// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GraphicsQualityManager.h"
#include "BlackHoleGameMode.generated.h"

/**
 * ABlackHoleGameMode
 *
 * Sets the default pawn, HUD, and game state classes.
 * Manages the game-level state machine (playing, ship destroyed, victory).
 */
UCLASS(BlueprintType, Blueprintable)
class BLACKHOLEGAME_API ABlackHoleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABlackHoleGameMode();

protected:
	virtual void BeginPlay() override;

public:
	/** Called when the spaceship is destroyed by tidal forces or crossing the horizon */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void OnSpaceshipDestroyed();

	/** Called to restart the level */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartGame();

	/** Auto GPU detection and quality preset manager */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
	TObjectPtr<UGraphicsQualityManager> GraphicsQualityManager;

	/** Whether the game is currently active */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
	bool bGameActive = true;

	/** Distance (km) at which the player is considered to have "arrived" at the black hole */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	float ArrivalDistanceKm = 5.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void OnGameOver(bool bWon);
};
