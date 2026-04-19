// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlackHoleActor.h"
#include "SpaceshipPawn.h"
#include "BlackHoleHUD.generated.h"

/**
 * ABlackHoleHUD
 *
 * Full heads-up display for the black hole game.
 *
 * Rendered using Canvas draw calls (no UMG dependency for simplicity).
 * Displays:
 *  - Distance to event horizon (km and rs units)
 *  - Speed (km/s and fraction of c)
 *  - Escape velocity at current position
 *  - Can-escape indicator (green/red)
 *  - Ship time vs Universal time clocks
 *  - Time dilation factor
 *  - Structural integrity bar
 *  - Tidal stress indicator
 *  - Gravitational acceleration in g
 *  - POINT OF NO RETURN warning
 *  - EVENT HORIZON DISTANCE countdown
 *  - Gravitational redshift (z)
 *  - Orbital state
 *  - Spaghettification state
 */
UCLASS(BlueprintType, Blueprintable)
class BLACKHOLEGAME_API ABlackHoleHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABlackHoleHUD();

protected:
	virtual void BeginPlay() override;

public:
	virtual void DrawHUD() override;

	// ── Configuration ────────────────────────────────────────────────────────

	/** Master opacity of the HUD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	float HUDOpacity = 0.85f;

	/** Font size scale factor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	float FontScale = 1.0f;

	/** Color for safe/nominal status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor SafeColor = FLinearColor(0.0f, 1.0f, 0.4f, 1.0f);

	/** Color for warning status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor WarningColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f);

	/** Color for critical/danger status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor DangerColor = FLinearColor(1.0f, 0.15f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FLinearColor TextColor = FLinearColor(0.8f, 0.95f, 1.0f, 1.0f);

private:
	// ── Cached data from last frame ────────────────────────────────────────────
	UPROPERTY()
	TObjectPtr<ABlackHoleActor> CachedBlackHole;

	UPROPERTY()
	TObjectPtr<ASpaceshipPawn> CachedShip;

	void FetchActors();

	// ── Drawing helpers ────────────────────────────────────────────────────────
	void DrawPanel(float X, float Y, float W, float H, FLinearColor Color, float Alpha = 0.4f);
	void DrawLabelValue(float X, float& Y, float LineH,
		const FString& Label, const FString& Value, FLinearColor ValueColor);
	void DrawProgressBar(float X, float Y, float W, float H,
		float Fraction, FLinearColor FillColor, const FString& Label);
	void DrawBigWarning(const FString& Text, FLinearColor Color);
	void DrawClock(float X, float Y, float LineH,
		const FString& Label, double TimeSeconds, FLinearColor Color);

	// ── HUD sections ──────────────────────────────────────────────────────────
	void DrawNavigationPanel(float X, float Y);
	void DrawPhysicsPanel(float X, float Y);
	void DrawTimePanel(float X, float Y);
	void DrawStatusPanel(float X, float Y);
	void DrawWarnings(float CX, float CY);
	void DrawCrosshair(float CX, float CY);

	// Helpers
	static FString FormatTime(double TotalSeconds);
	static FString FormatDistance(double Meters);
	static FString OrbitalStateString(EOrbitalState State);
	static FString SpaghStateString(ESpaghettificationState State);
	FLinearColor IntegrityColor(float Integrity) const;

	float ScreenW = 0.0f;
	float ScreenH = 0.0f;
	float LineHeight = 20.0f;
	bool bWarningPhototonFired = false;
	float WarningPulse = 0.0f;
};
