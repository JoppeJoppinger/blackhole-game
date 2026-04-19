// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "SpaceEnvironmentActor.h"
#include "SpaceshipPawn.h"
#include "BlackHolePhysicsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

using namespace BlackHolePhysics;

// ─── Spectral type distribution (roughly matches Milky Way statistics) ─────────
// O: 0.00003%  B: 0.13%  A: 0.6%  F: 3%  G: 7.6%  K: 12.1%  M: 76.45%
// We use a simplified weighted table for visual richness (add more O/B for beauty)
static const float SpectralWeights[] = { 0.5f, 3.0f, 6.0f, 10.0f, 15.0f, 20.0f, 45.5f };
static const uint8 NumSpectralClasses = 7;

ASpaceEnvironmentActor::ASpaceEnvironmentActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SkyboxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyboxMesh"));
	SetRootComponent(SkyboxMesh);
	SkyboxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyboxMesh->CastShadow = false;
	// Note: assign a large inverted sphere mesh (sphere with normals flipped inward) in editor
}

void ASpaceEnvironmentActor::BeginPlay()
{
	Super::BeginPlay();
	GenerateStarField();
}

void ASpaceEnvironmentActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bRelativisticAberration)
		UpdateAberration();
}

// ─── Star generation ──────────────────────────────────────────────────────────

void ASpaceEnvironmentActor::GenerateStarField()
{
	FRandomStream RNG(RandomSeed);
	Stars.SetNum(NumStars);

	// Build cumulative spectral weight table
	float CumulativeWeights[NumSpectralClasses];
	float TotalWeight = 0.0f;
	for (int32 i = 0; i < NumSpectralClasses; ++i)
		TotalWeight += SpectralWeights[i];
	float Accum = 0.0f;
	for (int32 i = 0; i < NumSpectralClasses; ++i)
	{
		Accum += SpectralWeights[i];
		CumulativeWeights[i] = Accum / TotalWeight;
	}

	for (int32 i = 0; i < NumStars; ++i)
	{
		FStarData& Star = Stars[i];

		// ── Random direction on unit sphere ───────────────────────────────────
		// Use Gaussian method for uniform distribution
		float Phi   = RNG.FRandRange(0.0f, 2.0f * PI);
		float CosTheta = RNG.FRandRange(-1.0f, 1.0f);
		float SinTheta = FMath::Sqrt(FMath::Max(0.0f, 1.0f - CosTheta * CosTheta));

		// Milky Way bias: concentrate ~40% of stars in the galactic plane band
		// (|galactic latitude| < 15°)
		if (RNG.FRandRange(0.0f, 1.0f) < 0.40f)
		{
			// Bias toward equatorial band
			float LatitudeBias = RNG.FRandRange(-0.26f, 0.26f); // ±15° in radians
			CosTheta = FMath::Sin(LatitudeBias);
			SinTheta = FMath::Sqrt(FMath::Max(0.0f, 1.0f - CosTheta * CosTheta));
		}

		Star.Direction = FVector(SinTheta * FMath::Cos(Phi),
		                        SinTheta * FMath::Sin(Phi),
		                        CosTheta);

		// ── Spectral class ────────────────────────────────────────────────────
		float ClassRoll = RNG.FRandRange(0.0f, 1.0f);
		Star.SpectralClass = NumSpectralClasses - 1;
		for (uint8 c = 0; c < NumSpectralClasses; ++c)
		{
			if (ClassRoll <= CumulativeWeights[c])
			{
				Star.SpectralClass = c;
				break;
			}
		}

		// ── Color from spectral class ─────────────────────────────────────────
		Star.Color = GetSpectralColor(Star.SpectralClass);

		// Add slight random variation to color temperature
		float Variation = RNG.FRandRange(-0.05f, 0.05f);
		Star.Color.R = FMath::Clamp(Star.Color.R + Variation, 0.0f, 1.0f);
		Star.Color.G = FMath::Clamp(Star.Color.G + Variation * 0.5f, 0.0f, 1.0f);
		Star.Color.B = FMath::Clamp(Star.Color.B - Variation, 0.0f, 1.0f);

		// ── Apparent magnitude (brightness) ──────────────────────────────────
		// O/B stars are intrinsically brighter
		float BaseMag = 0.3f + (static_cast<float>(Star.SpectralClass) / 6.0f) * 0.5f;
		Star.Magnitude = BaseMag + RNG.FRandRange(-0.2f, 0.2f);
		Star.Magnitude = FMath::Clamp(Star.Magnitude, 0.05f, 1.0f);
	}

	// Expose star data to skybox material via dynamic material instance
	// The skybox material reads star positions from a texture that would be
	// generated from this data. In practice, a Blueprint post-process would
	// write Stars[] into a UTexture2D and pass it to the skybox material.
	// This is scaffolded here; Blueprint can call GetStarData() to retrieve it.
	UE_LOG(LogTemp, Log, TEXT("SpaceEnvironment: Generated %d stars."), NumStars);
}

// ─── Spectral Colors ──────────────────────────────────────────────────────────
// Based on real stellar blackbody temperatures converted to sRGB approximations

FLinearColor ASpaceEnvironmentActor::GetSpectralColor(uint8 SpectralClass)
{
	// Temperatures: O≈30000K, B≈20000K, A≈8500K, F≈6500K, G≈5700K, K≈4500K, M≈3200K
	// RGB approximations via Planckian locus:
	switch (SpectralClass)
	{
		case 0: return FLinearColor(0.64f, 0.70f, 1.00f); // O: blue-violet
		case 1: return FLinearColor(0.70f, 0.80f, 1.00f); // B: blue-white
		case 2: return FLinearColor(0.85f, 0.90f, 1.00f); // A: white-blue
		case 3: return FLinearColor(1.00f, 0.97f, 0.85f); // F: yellow-white
		case 4: return FLinearColor(1.00f, 0.90f, 0.60f); // G: yellow (sun-like)
		case 5: return FLinearColor(1.00f, 0.65f, 0.30f); // K: orange
		case 6: return FLinearColor(1.00f, 0.25f, 0.08f); // M: red
		default: return FLinearColor::White;
	}
}

// ─── Relativistic Aberration ──────────────────────────────────────────────────

FVector ASpaceEnvironmentActor::ApplyRelativisticAberration(FVector Direction, FVector ShipVelocity)
{
	double speed = static_cast<double>(ShipVelocity.Size());
	if (speed < 1.0) return Direction; // negligible

	double beta = speed / C;
	beta = FMath::Min(beta, 0.9999);

	// Direction of motion (travel axis)
	FVector vHat = ShipVelocity / static_cast<float>(speed);

	// cos(theta) = dot(Direction, vHat)
	double cosTheta = static_cast<double>(FVector::DotProduct(Direction, vHat));

	// Relativistic aberration formula:
	// cos(theta') = (cos(theta) + beta) / (1 + beta * cos(theta))
	double cosThetaPrime = (cosTheta + beta) / (1.0 + beta * cosTheta);
	cosThetaPrime = FMath::Clamp(cosThetaPrime, -1.0, 1.0);

	// Reconstruct direction: preserve transverse component, adjust longitudinal
	// Transverse component of original direction
	FVector Transverse = Direction - vHat * static_cast<float>(cosTheta);
	double sinTheta = static_cast<double>(Transverse.Size());

	if (sinTheta < 1e-8)
	{
		// Star is aligned with velocity axis
		return cosThetaPrime > 0.0 ? vHat : -vHat;
	}

	FVector TransverseHat = Transverse / static_cast<float>(sinTheta);

	// sin(theta') from cos(theta')
	double sinThetaPrime = FMath::Sqrt(FMath::Max(0.0, 1.0 - cosThetaPrime * cosThetaPrime));

	FVector AberratedDir = vHat * static_cast<float>(cosThetaPrime)
		+ TransverseHat * static_cast<float>(sinThetaPrime);
	return AberratedDir.GetSafeNormal();
}

// ─── Update aberration each tick ──────────────────────────────────────────────

void ASpaceEnvironmentActor::UpdateAberration()
{
	if (!CachedShip && SpaceshipClass)
	{
		AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), SpaceshipClass);
		CachedShip = Cast<APawn>(Found);
	}
	if (!CachedShip) return;

	UBlackHolePhysicsComponent* PhysComp =
		CachedShip->FindComponentByClass<UBlackHolePhysicsComponent>();
	if (!PhysComp) return;

	FVector ShipVelocity = PhysComp->GravityVelocity;
	// Add player velocity component
	ASpaceshipPawn* Ship = Cast<ASpaceshipPawn>(CachedShip);
	if (Ship) ShipVelocity += Ship->PlayerVelocity;

	double speed = static_cast<double>(ShipVelocity.Size());
	CurrentBeta = static_cast<float>(speed / C);

	// Only update if beta is significant (> 0.01 = 1% of c)
	if (CurrentBeta > 0.01f)
	{
		// Pass velocity to skybox material for GPU-side aberration shader
		if (SkyboxMesh)
		{
			SkyboxMesh->SetVectorParameterValueOnMaterials(
				FName("ShipVelocity"),
				FVector(ShipVelocity.X, ShipVelocity.Y, ShipVelocity.Z));
			SkyboxMesh->SetScalarParameterValueOnMaterials(
				FName("BetaFactor"), CurrentBeta);
		}
	}
}
