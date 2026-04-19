// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "BlackHolePhysicsComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

using namespace BlackHolePhysics;

UBlackHolePhysicsComponent::UBlackHolePhysicsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBlackHolePhysicsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find the black hole if not set
	if (!BlackHole)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlackHoleActor::StaticClass(), FoundActors);
		if (FoundActors.Num() > 0)
		{
			BlackHole = Cast<ABlackHoleActor>(FoundActors[0]);
		}
	}
}

void UBlackHolePhysicsComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!BlackHole || !GetOwner()) return;
	if (SpaghettificationState == ESpaghettificationState::Destroyed) return;

	FVector OwnerPos = GetOwner()->GetActorLocation();

	// ── 1. Gravitational acceleration ─────────────────────────────────────────
	if (bGravityEnabled)
	{
		FVector GravAccel = BlackHole->GetGravitationalAcceleration(OwnerPos);
		CurrentGravAcceleration = GravAccel.Size();
		GravAccelInG = static_cast<float>(CurrentGravAcceleration / StandardGravity);

		// Accumulate gravity velocity
		GravityVelocity += GravAccel * DeltaTime;

		// Apply to owner location
		FVector NewLocation = OwnerPos + GravityVelocity * DeltaTime;
		GetOwner()->SetActorLocation(NewLocation, true);
	}

	// ── 2. Frame dragging ─────────────────────────────────────────────────────
	if (bFrameDraggingEnabled)
	{
		FVector FrameDragAccel = BlackHole->GetFrameDraggingForce(OwnerPos, GravityVelocity);
		GravityVelocity += FrameDragAccel * DeltaTime;
	}

	// ── 3. Tidal forces ───────────────────────────────────────────────────────
	FVector TidalAccel = BlackHole->GetTidalAcceleration(OwnerPos, ShipLength_m);
	CurrentTidalAcceleration = TidalAccel.Size();

	// ── 4. Update states ──────────────────────────────────────────────────────
	UpdateOrbitalState();
	UpdateSpaghettificationState(DeltaTime);
	UpdateTidalStretch();

	// ── 5. Check event horizon ────────────────────────────────────────────────
	if (BlackHole->IsInsideEventHorizon(OwnerPos))
	{
		if (SpaghettificationState != ESpaghettificationState::Destroyed)
		{
			SpaghettificationState = ESpaghettificationState::Destroyed;
			StructuralIntegrity = 0.0f;
			OnSpaghettificationStateChanged.Broadcast(ESpaghettificationState::Destroyed);
			OnShipDestroyed.Broadcast();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────

void UBlackHolePhysicsComponent::UpdateOrbitalState()
{
	if (!BlackHole || !GetOwner()) return;

	FVector OwnerPos = GetOwner()->GetActorLocation();
	double r = static_cast<double>((OwnerPos - BlackHole->GetActorLocation()).Size());
	double v_esc = BlackHole->GetEscapeVelocity(OwnerPos);
	double v_current = static_cast<double>(GravityVelocity.Size());

	if (r <= BlackHole->SchwarzschildRadius)
	{
		OrbitalState = EOrbitalState::Captured;
		return;
	}

	// Compute specific orbital energy: E = 0.5*v² - GM/r
	double GM_val = BlackHolePhysics::G * BlackHole->MassKg;
	double specificEnergy = 0.5 * v_current * v_current - GM_val / r;

	if (specificEnergy >= 0.0)
	{
		OrbitalState = EOrbitalState::Escape;
	}
	else
	{
		// Elliptical orbit; check if close to circular
		// Angular momentum per unit mass
		FVector r_vec = OwnerPos - BlackHole->GetActorLocation();
		FVector L = FVector::CrossProduct(r_vec, GravityVelocity);
		double Lmag = static_cast<double>(L.Size());

		// Circular orbit condition: v_circ = sqrt(GM/r)
		double v_circ = FMath::Sqrt(GM_val / r);
		double v_ratio = v_current / v_circ;

		if (FMath::Abs(v_ratio - 1.0) < 0.05)
			OrbitalState = EOrbitalState::Circular;
		else
			OrbitalState = EOrbitalState::Elliptical;
	}
}

ESpaghettificationState UBlackHolePhysicsComponent::ComputeNewSpaghettificationState() const
{
	// Thresholds relative to TidalStressThreshold
	float T = TidalStressThreshold;
	float tidal = CurrentTidalAcceleration;

	if (tidal < T * CriticalMultiplier)        return ESpaghettificationState::Normal;
	if (tidal < T * StretchingMultiplier)      return ESpaghettificationState::Stressed;
	if (tidal < T * 10.0f)                     return ESpaghettificationState::Stretching;
	if (tidal < T * 30.0f)                     return ESpaghettificationState::Critical;
	return ESpaghettificationState::Destroyed;
}

void UBlackHolePhysicsComponent::UpdateSpaghettificationState(float DeltaTime)
{
	ESpaghettificationState NewState = ComputeNewSpaghettificationState();

	if (NewState != SpaghettificationState)
	{
		SpaghettificationState = NewState;
		OnSpaghettificationStateChanged.Broadcast(NewState);

		if (NewState == ESpaghettificationState::Destroyed)
		{
			StructuralIntegrity = 0.0f;
			OnShipDestroyed.Broadcast();
		}
	}

	// Drain structural integrity based on tidal stress
	float StressDrain = 0.0f;
	switch (SpaghettificationState)
	{
		case ESpaghettificationState::Stressed:    StressDrain = 0.01f; break;
		case ESpaghettificationState::Stretching:  StressDrain = 0.05f; break;
		case ESpaghettificationState::Critical:    StressDrain = 0.20f; break;
		case ESpaghettificationState::Destroyed:   StressDrain = 1.00f; break;
		default: break;
	}
	StructuralIntegrity = FMath::Clamp(StructuralIntegrity - StressDrain * DeltaTime, 0.0f, 1.0f);
}

void UBlackHolePhysicsComponent::UpdateTidalStretch()
{
	if (!BlackHole || !GetOwner()) return;

	FVector OwnerPos = GetOwner()->GetActorLocation();
	FVector RadialDir = (OwnerPos - BlackHole->GetActorLocation()).GetSafeNormal();

	float StretchFactor = 1.0f;
	float CompressFactor = 1.0f;

	switch (SpaghettificationState)
	{
		case ESpaghettificationState::Stressed:
			StretchFactor = 1.05f; CompressFactor = 0.975f; break;
		case ESpaghettificationState::Stretching:
			StretchFactor = 1.25f; CompressFactor = 0.90f; break;
		case ESpaghettificationState::Critical:
			StretchFactor = 2.0f;  CompressFactor = 0.70f; break;
		case ESpaghettificationState::Destroyed:
			StretchFactor = 5.0f;  CompressFactor = 0.30f; break;
		default:
			TidalStretchScale = FVector(1.0f, 1.0f, 1.0f);
			return;
	}

	// Determine local radial axis and build stretch scale
	// We stretch along the direction toward the black hole, compress transversely
	FVector LocalRadial = GetOwner()->GetActorTransform().InverseTransformVector(RadialDir);
	// Approximate: if radial is mostly along X, stretch X; compress Y, Z
	TidalStretchScale = FVector(
		FMath::Lerp(1.0f, StretchFactor, FMath::Abs(LocalRadial.X)),
		FMath::Lerp(1.0f, CompressFactor, 1.0f - FMath::Abs(LocalRadial.X)),
		FMath::Lerp(1.0f, CompressFactor, 1.0f - FMath::Abs(LocalRadial.X))
	);
}
