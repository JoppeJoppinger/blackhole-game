// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "BlackHoleActor.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Kismet/GameplayStatics.h"

using namespace BlackHolePhysics;

ABlackHoleActor::ABlackHoleActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Event horizon visual sphere
	EventHorizonSphere = CreateDefaultSubobject<USphereComponent>(TEXT("EventHorizonSphere"));
	EventHorizonSphere->SetupAttachment(Root);
	EventHorizonSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EventHorizonSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	EventHorizonSphere->ShapeColor = FColor::Black;
	EventHorizonSphere->SetHiddenInGame(false);

	// Photon sphere visual (no collision, just visual reference)
	PhotonSphereBoundary = CreateDefaultSubobject<USphereComponent>(TEXT("PhotonSphereBoundary"));
	PhotonSphereBoundary->SetupAttachment(Root);
	PhotonSphereBoundary->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PhotonSphereBoundary->ShapeColor = FColor(255, 165, 0); // orange
	PhotonSphereBoundary->SetHiddenInGame(false);

	// ISCO visual
	ISCOBoundary = CreateDefaultSubobject<USphereComponent>(TEXT("ISCOBoundary"));
	ISCOBoundary->SetupAttachment(Root);
	ISCOBoundary->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISCOBoundary->ShapeColor = FColor::Red;
	ISCOBoundary->SetHiddenInGame(false);
}

void ABlackHoleActor::BeginPlay()
{
	Super::BeginPlay();
	ComputeDerivedConstants();
}

void ABlackHoleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ApplyGravityToPhysicsActors(DeltaTime);
}

// ─── Derived constants ────────────────────────────────────────────────────────

void ABlackHoleActor::ComputeDerivedConstants()
{
	MassKg = MassInSolarMasses * SolarMass;
	GM = G * MassKg;

	// Schwarzschild radius: rs = 2GM/c²
	SchwarzschildRadius = 2.0 * GM / C2;

	// Photon sphere: r_ph = 1.5 * rs  (Schwarzschild; shifts inward for Kerr prograde)
	// For Kerr: r_ph ≈ rs * (1 + cos(2/3 * arccos(-a)))  (prograde)
	// We use the Schwarzschild approximation for the outer photon sphere
	PhotonSphereRadius = 1.5 * SchwarzschildRadius;

	// ISCO (Innermost Stable Circular Orbit)
	// Schwarzschild: r_ISCO = 3 * rs = 6GM/c²
	// Kerr prograde: r_ISCO ≈ rs * Z2 (complex formula, approximated below)
	// Using the full Kerr ISCO formula (Bardeen, Press, Teukolsky 1972):
	double a = SpinParameter; // dimensionless spin 0..1
	double Z1 = 1.0 + FMath::Pow(1.0 - a * a, 1.0 / 3.0)
		* (FMath::Pow(1.0 + a, 1.0 / 3.0) + FMath::Pow(1.0 - a, 1.0 / 3.0));
	double Z2 = FMath::Sqrt(3.0 * a * a + Z1 * Z1);
	// Prograde ISCO in units of rs/2 = GM/c²:
	double r_ISCO_units = 3.0 + Z2 - FMath::Sqrt((3.0 - Z1) * (3.0 + Z1 + 2.0 * Z2));
	ISCORadius = r_ISCO_units * (SchwarzschildRadius / 2.0);

	// Update sphere radii (UU = meters)
	if (EventHorizonSphere)
		EventHorizonSphere->SetSphereRadius(static_cast<float>(SchwarzschildRadius));
	if (PhotonSphereBoundary)
		PhotonSphereBoundary->SetSphereRadius(static_cast<float>(PhotonSphereRadius));
	if (ISCOBoundary)
		ISCOBoundary->SetSphereRadius(static_cast<float>(ISCORadius));

	UE_LOG(LogTemp, Log,
		TEXT("BlackHole: M=%.2e kg, rs=%.1f m, r_ph=%.1f m, r_ISCO=%.1f m"),
		MassKg, SchwarzschildRadius, PhotonSphereRadius, ISCORadius);
}

// ─── Gravitational acceleration ───────────────────────────────────────────────

FVector ABlackHoleActor::GetGravitationalAcceleration(FVector WorldPosition) const
{
	FVector Delta = GetActorLocation() - WorldPosition; // toward BH
	double r = static_cast<double>(Delta.Size());
	if (r < SchwarzschildRadius * 0.01) r = SchwarzschildRadius * 0.01; // avoid singularity

	// a = GM/r²  (Newtonian; GR correction would require geodesic integration)
	double aMag = GM / (r * r);
	FVector Dir = Delta / static_cast<float>(r);
	return Dir * static_cast<float>(aMag);
}

// ─── Tidal acceleration ───────────────────────────────────────────────────────

FVector ABlackHoleActor::GetTidalAcceleration(FVector WorldPosition, float ObjectLength_m) const
{
	FVector Delta = GetActorLocation() - WorldPosition;
	double r = static_cast<double>(Delta.Size());
	if (r < SchwarzschildRadius * 0.01) r = SchwarzschildRadius * 0.01;

	// Tidal acceleration along radial axis: a_tidal = 2GM * l / r³
	double l = static_cast<double>(ObjectLength_m);
	double tidalMag = 2.0 * GM * l / (r * r * r);

	FVector RadialDir = Delta.GetSafeNormal();
	return RadialDir * static_cast<float>(tidalMag);
}

// ─── Time dilation ────────────────────────────────────────────────────────────

double ABlackHoleActor::GetTimeDilationFactor(FVector WorldPosition) const
{
	FVector Delta = WorldPosition - GetActorLocation();
	double r = static_cast<double>(Delta.Size());
	if (r <= SchwarzschildRadius) return 0.0; // at or inside horizon

	double factor = FMath::Sqrt(1.0 - SchwarzschildRadius / r);
	return FMath::Clamp(factor, 0.0, 1.0);
}

// ─── Frame dragging (Lense-Thirring) ─────────────────────────────────────────

FVector ABlackHoleActor::GetFrameDraggingForce(FVector WorldPosition, FVector Velocity) const
{
	// Lense-Thirring precession: Omega_LT = G*J / (c² * r³) * (3*(J_hat·r_hat)*r_hat - J_hat)
	// Angular momentum magnitude: J = a * M * c * rs/2  (dimensionful)
	double J = SpinParameter * MassKg * C * (SchwarzschildRadius / 2.0);

	FVector r_vec = WorldPosition - GetActorLocation();
	double r = static_cast<double>(r_vec.Size());
	if (r < SchwarzschildRadius) return FVector::ZeroVector;

	FVector r_hat = r_vec.GetSafeNormal();
	FVector J_hat = SpinAxis.GetSafeNormal();

	// Precession angular velocity vector
	double prefactor = G * J / (C2 * r * r * r);
	FVector Omega = (J_hat * 3.0f * static_cast<float>(FVector::DotProduct(FVector(J_hat), r_hat)) - J_hat)
		* static_cast<float>(prefactor);

	// Frame drag force = Omega x Velocity (Coriolis-like term)
	FVector FrameDragAccel = FVector::CrossProduct(Omega, Velocity) * 2.0f;
	return FrameDragAccel;
}

// ─── Escape velocity ──────────────────────────────────────────────────────────

double ABlackHoleActor::GetEscapeVelocity(FVector WorldPosition) const
{
	FVector Delta = WorldPosition - GetActorLocation();
	double r = static_cast<double>(Delta.Size());
	if (r <= SchwarzschildRadius) return C; // can't escape from inside

	// v_esc = sqrt(2GM/r)
	return FMath::Sqrt(2.0 * GM / r);
}

// ─── Gravitational redshift ───────────────────────────────────────────────────

double ABlackHoleActor::GetGravitationalRedshift(FVector WorldPosition) const
{
	FVector Delta = WorldPosition - GetActorLocation();
	double r = static_cast<double>(Delta.Size());
	if (r <= SchwarzschildRadius) return 1e9; // near-infinite at horizon

	double sqrtFactor = FMath::Sqrt(1.0 - SchwarzschildRadius / r);
	if (sqrtFactor < 1e-10) return 1e9;
	return (1.0 / sqrtFactor) - 1.0;
}

// ─── Distance to event horizon ────────────────────────────────────────────────

double ABlackHoleActor::GetDistanceToEventHorizon(FVector WorldPosition) const
{
	double r = static_cast<double>((WorldPosition - GetActorLocation()).Size());
	return r - SchwarzschildRadius;
}

bool ABlackHoleActor::IsInsideEventHorizon(FVector WorldPosition) const
{
	return GetDistanceToEventHorizon(WorldPosition) < 0.0;
}

// ─── Apply gravity to all physics primitives in scene ─────────────────────────

void ABlackHoleActor::ApplyGravityToPhysicsActors(float DeltaTime)
{
	// Only affect objects within 5 million km (5e9 m) for performance
	const double MaxInfluenceRadius = 5.0e9;
	FVector BHLocation = GetActorLocation();

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this) continue;

		// Only affect actors with simulating physics components
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
		if (!PrimComp || !PrimComp->IsSimulatingPhysics()) continue;

		FVector ActorPos = Actor->GetActorLocation();
		double Dist = static_cast<double>((ActorPos - BHLocation).Size());
		if (Dist > MaxInfluenceRadius) continue;

		FVector GravAccel = GetGravitationalAcceleration(ActorPos);
		// Convert acceleration to force: F = m*a, but AddForce handles mass internally
		// We use AddWorldOffset for lightweight simulation, or AddForce for physics bodies
		FVector Impulse = GravAccel * DeltaTime;
		PrimComp->AddImpulse(Impulse * PrimComp->GetMass(), NAME_None, true);
	}
}
