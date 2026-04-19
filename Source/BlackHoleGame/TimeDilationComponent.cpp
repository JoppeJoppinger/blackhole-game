// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "TimeDilationComponent.h"
#include "BlackHoleActor.h"
#include "BlackHolePhysicsComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

using namespace BlackHolePhysics;

UTimeDilationComponent::UTimeDilationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTimeDilationComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetClocks();
}

void UTimeDilationComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner()) return;

	double dt = static_cast<double>(DeltaTime);

	// ── Gravitational dilation ────────────────────────────────────────────────
	GravitationalDilationFactor = 1.0;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlackHoleActor::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		ABlackHoleActor* BH = Cast<ABlackHoleActor>(FoundActors[0]);
		if (BH)
		{
			GravitationalDilationFactor = BH->GetTimeDilationFactor(GetOwner()->GetActorLocation());
		}
	}

	// ── Kinematic dilation (special relativistic) ─────────────────────────────
	FVector Velocity = FVector::ZeroVector;
	UBlackHolePhysicsComponent* PhysComp =
		GetOwner()->FindComponentByClass<UBlackHolePhysicsComponent>();
	if (PhysComp)
	{
		Velocity = PhysComp->GravityVelocity;

		// Add player-input velocity (from SpaceshipPawn)
		// The pawn velocity is accessible through the owner's velocity
		if (GetOwner()->GetRootComponent())
		{
			// We read total world velocity from the owner's movement if available
		}
	}

	double speed = static_cast<double>(Velocity.Size()); // m/s
	SpeedKmPerSecond = speed / 1000.0;
	SpeedFractionOfC = speed / C;

	// Clamp below c to avoid NaN (game has no speed limit but physics break at c)
	double beta2 = FMath::Min(SpeedFractionOfC * SpeedFractionOfC, 0.9999);
	KinematicDilationFactor = FMath::Sqrt(1.0 - beta2);

	// ── Combined ──────────────────────────────────────────────────────────────
	TotalDilationFactor = GravitationalDilationFactor * KinematicDilationFactor;

	// Accumulate clocks
	CoordinateTimeAccumulated += dt;
	ProperTimeAccumulated += dt * TotalDilationFactor;

	// Broadcast if changed significantly
	if (FMath::Abs(TotalDilationFactor - LastBroadcastDilationFactor) > DilationChangeBroadcastThreshold)
	{
		double ProperDelta = dt * TotalDilationFactor;
		OnTimeDilationChanged.Broadcast(TotalDilationFactor, ProperDelta);
		LastBroadcastDilationFactor = TotalDilationFactor;
	}
}

void UTimeDilationComponent::ResetClocks()
{
	ProperTimeAccumulated = 0.0;
	CoordinateTimeAccumulated = 0.0;
	LastBroadcastDilationFactor = 1.0;
}
