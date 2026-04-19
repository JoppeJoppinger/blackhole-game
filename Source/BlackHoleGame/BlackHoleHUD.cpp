// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "BlackHoleHUD.h"
#include "TimeDilationComponent.h"
#include "BlackHolePhysicsComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ABlackHoleHUD::ABlackHoleHUD()
{
}

void ABlackHoleHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ABlackHoleHUD::FetchActors()
{
	if (!CachedBlackHole)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlackHoleActor::StaticClass(), Found);
		if (Found.Num() > 0)
			CachedBlackHole = Cast<ABlackHoleActor>(Found[0]);
	}
	if (!CachedShip)
	{
		APawn* P = GetOwningPawn();
		CachedShip = Cast<ASpaceshipPawn>(P);
	}
}

// ─── Main draw entry ──────────────────────────────────────────────────────────

void ABlackHoleHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	if (CachedShip && !CachedShip->bHUDVisible) return;

	FetchActors();

	ScreenW = Canvas->ClipX;
	ScreenH = Canvas->ClipY;
	LineHeight = FMath::RoundToFloat(ScreenH * 0.022f * FontScale);

	WarningPulse += GetWorld()->GetDeltaSeconds() * 3.0f;

	// ── Panels ────────────────────────────────────────────────────────────────
	// Left column: Navigation
	DrawNavigationPanel(20.0f, 20.0f);

	// Right column: Physics
	DrawPhysicsPanel(ScreenW - 320.0f, 20.0f);

	// Bottom-left: Time
	DrawTimePanel(20.0f, ScreenH - 200.0f);

	// Bottom-right: Status
	DrawStatusPanel(ScreenW - 320.0f, ScreenH - 220.0f);

	// Centre warnings
	DrawWarnings(ScreenW * 0.5f, ScreenH * 0.35f);

	// Crosshair
	DrawCrosshair(ScreenW * 0.5f, ScreenH * 0.5f);
}

// ─── Navigation Panel ─────────────────────────────────────────────────────────

void ABlackHoleHUD::DrawNavigationPanel(float X, float Y)
{
	const float W = 300.0f, H = 220.0f;
	DrawPanel(X, Y, W, H, FLinearColor(0.0f, 0.05f, 0.15f));
	DrawLabelValue(X + 8, Y, LineHeight, "═ NAVIGATION ═", "", TextColor);
	Y += LineHeight * 1.3f;

	if (!CachedBlackHole || !CachedShip)
	{
		DrawLabelValue(X + 8, Y, LineHeight, "No black hole detected", "", WarningColor);
		return;
	}

	FVector ShipPos = CachedShip->GetActorLocation();
	double distToHorizon = CachedBlackHole->GetDistanceToEventHorizon(ShipPos);
	double rs = CachedBlackHole->SchwarzschildRadius;
	double r = static_cast<double>((ShipPos - CachedBlackHole->GetActorLocation()).Size());

	// Distance to event horizon
	FLinearColor distColor = (distToHorizon < rs * 2.0) ? DangerColor :
	                         (distToHorizon < rs * 5.0) ? WarningColor : SafeColor;
	DrawLabelValue(X + 8, Y, LineHeight,
		"Dist to Horizon:", FormatDistance(distToHorizon), distColor);

	// Distance in Schwarzschild radii
	double r_in_rs = r / rs;
	DrawLabelValue(X + 8, Y, LineHeight,
		"Distance (rs):",
		FString::Printf(TEXT("%.2f rs"), r_in_rs),
		TextColor);

	// Escape velocity
	double v_esc = CachedBlackHole->GetEscapeVelocity(ShipPos);
	DrawLabelValue(X + 8, Y, LineHeight,
		"Escape Velocity:",
		FString::Printf(TEXT("%.1f km/s"), v_esc / 1000.0),
		TextColor);

	// Can-escape indicator
	double totalSpeed = static_cast<double>(CachedShip->TotalVelocity.Size());
	bool bCanEscape = (totalSpeed >= v_esc) && !CachedBlackHole->IsInsideEventHorizon(ShipPos);
	FLinearColor escapeColor = bCanEscape ? SafeColor : DangerColor;
	FString escapeStr = bCanEscape ? TEXT("YES ▲") : TEXT("NO ▼");
	DrawLabelValue(X + 8, Y, LineHeight, "Can Escape:", escapeStr, escapeColor);

	// Orbital state
	EOrbitalState OrbState = EOrbitalState::Escape;
	UBlackHolePhysicsComponent* PhysComp =
		CachedShip->FindComponentByClass<UBlackHolePhysicsComponent>();
	if (PhysComp) OrbState = PhysComp->OrbitalState;
	DrawLabelValue(X + 8, Y, LineHeight,
		"Orbit:", OrbitalStateString(OrbState), TextColor);

	// Photon sphere / ISCO indicators
	if (r <= CachedBlackHole->PhotonSphereRadius)
	{
		DrawLabelValue(X + 8, Y, LineHeight,
			"ZONE:", TEXT("PHOTON SPHERE"), DangerColor);
	}
	else if (r <= CachedBlackHole->ISCORadius)
	{
		DrawLabelValue(X + 8, Y, LineHeight,
			"ZONE:", TEXT("INSIDE ISCO"), WarningColor);
	}
}

// ─── Physics Panel ────────────────────────────────────────────────────────────

void ABlackHoleHUD::DrawPhysicsPanel(float X, float Y)
{
	const float W = 300.0f, H = 260.0f;
	DrawPanel(X, Y, W, H, FLinearColor(0.1f, 0.0f, 0.05f));
	DrawLabelValue(X + 8, Y, LineHeight, "═ PHYSICS ═", "", TextColor);
	Y += LineHeight * 1.3f;

	if (!CachedShip) return;

	UBlackHolePhysicsComponent* PhysComp =
		CachedShip->FindComponentByClass<UBlackHolePhysicsComponent>();

	// Speed
	DrawLabelValue(X + 8, Y, LineHeight,
		"Speed:",
		FString::Printf(TEXT("%.2f km/s"), CachedShip->SpeedKmPerSec),
		TextColor);
	DrawLabelValue(X + 8, Y, LineHeight,
		"Speed (c):",
		FString::Printf(TEXT("%.6f c"), CachedShip->SpeedFractionOfC),
		CachedShip->SpeedFractionOfC > 0.5f ? WarningColor : TextColor);

	if (PhysComp && CachedBlackHole)
	{
		// Gravitational acceleration
		DrawLabelValue(X + 8, Y, LineHeight,
			"Gravity:",
			FString::Printf(TEXT("%.3f g"), PhysComp->GravAccelInG),
			PhysComp->GravAccelInG > 100.0f ? DangerColor : TextColor);

		// Tidal acceleration
		float TidalG = PhysComp->CurrentTidalAcceleration / 9.80665f;
		DrawLabelValue(X + 8, Y, LineHeight,
			"Tidal Force:",
			FString::Printf(TEXT("%.4f g/m"), TidalG),
			TidalG > 1.0f ? WarningColor : TextColor);

		// Redshift
		FVector ShipPos = CachedShip->GetActorLocation();
		double z = CachedBlackHole->GetGravitationalRedshift(ShipPos);
		FLinearColor zColor = (z > 1.0) ? DangerColor : (z > 0.1) ? WarningColor : TextColor;
		DrawLabelValue(X + 8, Y, LineHeight,
			"Grav. Redshift (z):",
			(z > 999.0) ? TEXT("∞") : FString::Printf(TEXT("%.4f"), z),
			zColor);

		// Time dilation factor
		UTimeDilationComponent* TimeComp =
			CachedShip->FindComponentByClass<UTimeDilationComponent>();
		if (TimeComp)
		{
			DrawLabelValue(X + 8, Y, LineHeight,
				"Time Dilation:",
				FString::Printf(TEXT("×%.6f"), TimeComp->TotalDilationFactor),
				TimeComp->TotalDilationFactor < 0.5 ? WarningColor : TextColor);
		}
	}

	// Spaghettification
	if (PhysComp)
	{
		FLinearColor spColor = TextColor;
		ESpaghettificationState SpState = PhysComp->SpaghettificationState;
		if (SpState == ESpaghettificationState::Stretching || SpState == ESpaghettificationState::Critical)
			spColor = DangerColor;
		else if (SpState == ESpaghettificationState::Stressed)
			spColor = WarningColor;

		DrawLabelValue(X + 8, Y, LineHeight,
			"Spaghettification:", SpaghStateString(SpState), spColor);
	}
}

// ─── Time Panel ───────────────────────────────────────────────────────────────

void ABlackHoleHUD::DrawTimePanel(float X, float Y)
{
	if (!CachedShip) return;
	UTimeDilationComponent* TimeComp =
		CachedShip->FindComponentByClass<UTimeDilationComponent>();
	if (!TimeComp) return;

	const float W = 300.0f, H = 170.0f;
	DrawPanel(X, Y, W, H, FLinearColor(0.0f, 0.08f, 0.05f));
	DrawLabelValue(X + 8, Y, LineHeight, "═ TIME ═", "", TextColor);
	Y += LineHeight * 1.3f;

	// Ship clock (proper time — ticks slower near BH)
	DrawLabelValue(X + 8, Y, LineHeight,
		"Ship Clock:",
		FormatTime(TimeComp->ProperTimeAccumulated),
		SafeColor);

	// Universal clock (coordinate time)
	DrawLabelValue(X + 8, Y, LineHeight,
		"Universe Clock:",
		FormatTime(TimeComp->CoordinateTimeAccumulated),
		FLinearColor(0.8f, 0.8f, 1.0f));

	// How much time has passed outside vs inside
	double ratio = (TimeComp->CoordinateTimeAccumulated > 0.001)
		? TimeComp->ProperTimeAccumulated / TimeComp->CoordinateTimeAccumulated
		: 1.0;
	DrawLabelValue(X + 8, Y, LineHeight,
		"Proper/Coord Ratio:",
		FString::Printf(TEXT("%.6f"), ratio),
		ratio < 0.9 ? WarningColor : TextColor);

	// Gravitational dilation breakdown
	DrawLabelValue(X + 8, Y, LineHeight,
		"  Gravitational:",
		FString::Printf(TEXT("×%.6f"), TimeComp->GravitationalDilationFactor),
		TextColor);
	DrawLabelValue(X + 8, Y, LineHeight,
		"  Kinematic (SR):",
		FString::Printf(TEXT("×%.6f"), TimeComp->KinematicDilationFactor),
		TextColor);
}

// ─── Status Panel ─────────────────────────────────────────────────────────────

void ABlackHoleHUD::DrawStatusPanel(float X, float Y)
{
	const float W = 300.0f, H = 200.0f;
	DrawPanel(X, Y, W, H, FLinearColor(0.08f, 0.0f, 0.0f));
	DrawLabelValue(X + 8, Y, LineHeight, "═ SHIP STATUS ═", "", TextColor);
	Y += LineHeight * 1.3f;

	if (!CachedShip) return;

	// Structural Integrity bar
	float Integrity = CachedShip->StructuralIntegrity;
	DrawProgressBar(X + 8, Y, 280.0f, 16.0f,
		Integrity, IntegrityColor(Integrity), "Hull Integrity");
	Y += 28.0f;

	// Tidal stress bar
	UBlackHolePhysicsComponent* PhysComp =
		CachedShip->FindComponentByClass<UBlackHolePhysicsComponent>();
	if (PhysComp)
	{
		float tidalNorm = FMath::Clamp(
			PhysComp->CurrentTidalAcceleration / (PhysComp->TidalStressThreshold * 30.0f), 0.0f, 1.0f);
		DrawProgressBar(X + 8, Y, 280.0f, 16.0f,
			tidalNorm,
			tidalNorm > 0.8f ? DangerColor : tidalNorm > 0.4f ? WarningColor : SafeColor,
			"Tidal Stress");
		Y += 28.0f;
	}

	// Camera mode
	DrawLabelValue(X + 8, Y, LineHeight,
		"Camera:",
		CachedShip->CameraMode == ECameraMode::Cockpit ? TEXT("Cockpit [V]") : TEXT("Chase [V]"),
		TextColor);

	// Controls reminder
	Y += LineHeight * 0.5f;
	FString ControlHint = TEXT("W/S=Thrust  A/D=Strafe  Q/E=Roll  Mouse=Look");
	Canvas->SetDrawColor(FColor(150, 180, 200, 120));
	Canvas->DrawText(GEngine->GetSmallFont(), ControlHint, X + 8, Y, 0.75f * FontScale, 0.75f * FontScale);
}

// ─── Warnings ─────────────────────────────────────────────────────────────────

void ABlackHoleHUD::DrawWarnings(float CX, float CY)
{
	if (!CachedBlackHole || !CachedShip) return;

	FVector ShipPos = CachedShip->GetActorLocation();
	double r = static_cast<double>((ShipPos - CachedBlackHole->GetActorLocation()).Size());
	double distToHorizon = CachedBlackHole->GetDistanceToEventHorizon(ShipPos);

	// Pulsing alpha
	float PulseAlpha = 0.6f + 0.4f * FMath::Sin(WarningPulse);

	// ── POINT OF NO RETURN (photon sphere) ────────────────────────────────────
	if (r <= CachedBlackHole->PhotonSphereRadius)
	{
		FLinearColor WarnColor = DangerColor;
		WarnColor.A = PulseAlpha;
		DrawBigWarning(TEXT("⚠  POINT OF NO RETURN  ⚠"), WarnColor);
		CY += LineHeight * 3.0f;
	}

	// ── EVENT HORIZON COUNTDOWN ───────────────────────────────────────────────
	if (distToHorizon > 0.0 && distToHorizon < 500.0e3) // within 500 km
	{
		FLinearColor HorizonColor = DangerColor;
		HorizonColor.A = PulseAlpha;
		FString HorizonStr = FString::Printf(
			TEXT("EVENT HORIZON IN %s"), *FormatDistance(distToHorizon));

		float TW = Canvas->ClipX * 0.5f;
		float TX = CX - TW * 0.5f;
		Canvas->SetDrawColor(HorizonColor.ToFColor(true));
		Canvas->DrawText(GEngine->GetMediumFont(), HorizonStr,
			TX, CY, 1.2f * FontScale, 1.2f * FontScale);
		CY += LineHeight * 2.5f;
	}

	// ── SPAGHETTIFICATION WARNING ─────────────────────────────────────────────
	UBlackHolePhysicsComponent* PhysComp =
		CachedShip->FindComponentByClass<UBlackHolePhysicsComponent>();
	if (PhysComp)
	{
		if (PhysComp->SpaghettificationState == ESpaghettificationState::Critical)
		{
			FLinearColor CritColor = DangerColor;
			CritColor.A = PulseAlpha;
			DrawBigWarning(TEXT("⚠  STRUCTURAL FAILURE IMMINENT  ⚠"), CritColor);
		}
		else if (PhysComp->SpaghettificationState == ESpaghettificationState::Stretching)
		{
			DrawBigWarning(TEXT("HULL STRESS: STRETCHING"), WarningColor);
		}
	}

	// ── INSIDE EVENT HORIZON ──────────────────────────────────────────────────
	if (CachedBlackHole->IsInsideEventHorizon(ShipPos))
	{
		FLinearColor BeyondColor(1.0f, 0.0f, 0.0f, 1.0f);
		DrawBigWarning(TEXT("YOU HAVE CROSSED THE EVENT HORIZON"), BeyondColor);
		DrawBigWarning(TEXT("ESCAPE IS IMPOSSIBLE"), BeyondColor);
	}
}

// ─── Crosshair ────────────────────────────────────────────────────────────────

void ABlackHoleHUD::DrawCrosshair(float CX, float CY)
{
	const float Len = 12.0f, Gap = 4.0f, T = 1.5f;
	FLinearColor C(0.7f, 0.9f, 1.0f, 0.6f);
	FColor Col = C.ToFColor(true);

	Canvas->SetDrawColor(Col);
	// Horizontal
	Canvas->DrawTile(Canvas->ResolveUV(nullptr, 0, 0),
		CX - Len - Gap, CY - T * 0.5f, Len, T, 0, 0, 1, 1);
	Canvas->DrawTile(Canvas->ResolveUV(nullptr, 0, 0),
		CX + Gap, CY - T * 0.5f, Len, T, 0, 0, 1, 1);
	// Vertical
	Canvas->DrawTile(Canvas->ResolveUV(nullptr, 0, 0),
		CX - T * 0.5f, CY - Len - Gap, T, Len, 0, 0, 1, 1);
	Canvas->DrawTile(Canvas->ResolveUV(nullptr, 0, 0),
		CX - T * 0.5f, CY + Gap, T, Len, 0, 0, 1, 1);
}

// ─── Drawing Primitives ───────────────────────────────────────────────────────

void ABlackHoleHUD::DrawPanel(float X, float Y, float W, float H,
	FLinearColor Color, float Alpha)
{
	Color.A = Alpha * HUDOpacity;
	Canvas->SetDrawColor(Color.ToFColor(true));
	Canvas->DrawTile(Canvas->ResolveUV(nullptr, 0, 0), X, Y, W, H, 0, 0, 1, 1);
}

void ABlackHoleHUD::DrawLabelValue(float X, float& Y, float LineH,
	const FString& Label, const FString& Value, FLinearColor ValueColor)
{
	UFont* Font = GEngine->GetSmallFont();

	// Label in dim white
	Canvas->SetDrawColor(FColor(180, 190, 200, static_cast<uint8>(200 * HUDOpacity)));
	Canvas->DrawText(Font, Label, X, Y, FontScale, FontScale);

	// Value in provided color
	if (!Value.IsEmpty())
	{
		Canvas->SetDrawColor(ValueColor.ToFColor(true));
		float LW, LH;
		Canvas->TextSize(Font, Label, LW, LH, FontScale, FontScale);
		Canvas->DrawText(Font, Value, X + LW + 6.0f, Y, FontScale, FontScale);
	}
	Y += LineH;
}

void ABlackHoleHUD::DrawProgressBar(float X, float Y, float W, float H,
	float Fraction, FLinearColor FillColor, const FString& Label)
{
	// Background
	Canvas->SetDrawColor(FColor(30, 30, 40, 180));
	Canvas->DrawTile(Canvas->ResolveUV(nullptr, 0, 0), X, Y, W, H, 0, 0, 1, 1);

	// Fill
	Fraction = FMath::Clamp(Fraction, 0.0f, 1.0f);
	FillColor.A = HUDOpacity;
	Canvas->SetDrawColor(FillColor.ToFColor(true));
	Canvas->DrawTile(Canvas->ResolveUV(nullptr, 0, 0), X + 1, Y + 1,
		(W - 2) * Fraction, H - 2, 0, 0, 1, 1);

	// Label
	Canvas->SetDrawColor(FColor(220, 230, 240, 200));
	Canvas->DrawText(GEngine->GetSmallFont(), Label, X + 4, Y + 1, 0.85f * FontScale, 0.85f * FontScale);
}

void ABlackHoleHUD::DrawBigWarning(const FString& Text, FLinearColor Color)
{
	if (!Canvas) return;
	UFont* Font = GEngine->GetMediumFont();
	float TW, TH;
	Canvas->TextSize(Font, Text, TW, TH, 1.3f * FontScale, 1.3f * FontScale);
	float TX = (ScreenW - TW) * 0.5f;
	float TY = ScreenH * 0.12f;

	Canvas->SetDrawColor(Color.ToFColor(true));
	Canvas->DrawText(Font, Text, TX, TY, 1.3f * FontScale, 1.3f * FontScale);
}

void ABlackHoleHUD::DrawClock(float X, float Y, float LineH,
	const FString& Label, double TimeSeconds, FLinearColor Color)
{
	FString TimeStr = FormatTime(TimeSeconds);
	DrawLabelValue(X, Y, LineH, Label, TimeStr, Color);
}

// ─── Formatters ───────────────────────────────────────────────────────────────

FString ABlackHoleHUD::FormatTime(double TotalSeconds)
{
	bool bNeg = TotalSeconds < 0;
	TotalSeconds = FMath::Abs(TotalSeconds);
	int32 Days  = static_cast<int32>(TotalSeconds / 86400.0);
	int32 Hours = static_cast<int32>(FMath::Fmod(TotalSeconds, 86400.0) / 3600.0);
	int32 Mins  = static_cast<int32>(FMath::Fmod(TotalSeconds, 3600.0) / 60.0);
	double Secs = FMath::Fmod(TotalSeconds, 60.0);

	FString Result;
	if (Days > 0)
		Result = FString::Printf(TEXT("%dd %02d:%02d:%05.2f"), Days, Hours, Mins, Secs);
	else
		Result = FString::Printf(TEXT("%02d:%02d:%06.3f"), Hours, Mins, Secs);
	return bNeg ? TEXT("-") + Result : Result;
}

FString ABlackHoleHUD::FormatDistance(double Meters)
{
	if (Meters >= 1.0e9)
		return FString::Printf(TEXT("%.3f Gm"), Meters / 1.0e9);
	if (Meters >= 1.0e6)
		return FString::Printf(TEXT("%.3f Mm"), Meters / 1.0e6);
	if (Meters >= 1.0e3)
		return FString::Printf(TEXT("%.2f km"), Meters / 1.0e3);
	return FString::Printf(TEXT("%.1f m"), Meters);
}

FString ABlackHoleHUD::OrbitalStateString(EOrbitalState State)
{
	switch (State)
	{
		case EOrbitalState::Escape:     return TEXT("Escape Trajectory");
		case EOrbitalState::Elliptical: return TEXT("Elliptical Orbit");
		case EOrbitalState::Circular:   return TEXT("Circular Orbit");
		case EOrbitalState::Captured:   return TEXT("Captured — Falling");
		default:                        return TEXT("Unknown");
	}
}

FString ABlackHoleHUD::SpaghStateString(ESpaghettificationState State)
{
	switch (State)
	{
		case ESpaghettificationState::Normal:     return TEXT("Normal");
		case ESpaghettificationState::Stressed:   return TEXT("Stressed");
		case ESpaghettificationState::Stretching: return TEXT("Stretching!");
		case ESpaghettificationState::Critical:   return TEXT("CRITICAL!");
		case ESpaghettificationState::Destroyed:  return TEXT("DESTROYED");
		default:                                  return TEXT("Unknown");
	}
}

FLinearColor ABlackHoleHUD::IntegrityColor(float Integrity) const
{
	if (Integrity > 0.6f) return SafeColor;
	if (Integrity > 0.3f) return WarningColor;
	return DangerColor;
}
