// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "BlackHoleGameMode.h"
#include "SpaceshipPawn.h"
#include "BlackHoleHUD.h"
#include "GraphicsQualityManager.h"
#include "Kismet/GameplayStatics.h"

ABlackHoleGameMode::ABlackHoleGameMode()
{
	DefaultPawnClass = ASpaceshipPawn::StaticClass();
	HUDClass = ABlackHoleHUD::StaticClass();

	// Attach quality manager — runs GPU detection on BeginPlay
	GraphicsQualityManager = CreateDefaultSubobject<UGraphicsQualityManager>(TEXT("GraphicsQualityManager"));
}

void ABlackHoleGameMode::BeginPlay()
{
	Super::BeginPlay();
	bGameActive = true;
}

void ABlackHoleGameMode::OnSpaceshipDestroyed()
{
	if (!bGameActive) return;
	bGameActive = false;
	OnGameOver(false);
}

void ABlackHoleGameMode::RestartGame()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), true);
}
