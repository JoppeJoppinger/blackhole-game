// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "SpaceshipPawn.h"
#include "BlackHoleActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

using namespace BlackHolePhysics;

ASpaceshipPawn::ASpaceshipPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// ── Ship mesh ─────────────────────────────────────────────────────────────
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	SetRootComponent(ShipMesh);
	ShipMesh->SetSimulatePhysics(false); // We handle physics manually
	ShipMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// ── Chase camera ──────────────────────────────────────────────────────────
	ChaseSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseSpringArm"));
	ChaseSpringArm->SetupAttachment(ShipMesh);
	ChaseSpringArm->TargetArmLength = 150.0f;
	ChaseSpringArm->bUsePawnControlRotation = false;
	ChaseSpringArm->bInheritPitch = true;
	ChaseSpringArm->bInheritYaw = true;
	ChaseSpringArm->bInheritRoll = true;
	ChaseSpringArm->SetRelativeLocation(FVector(-250.0f, 0.0f, 50.0f));

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(ChaseSpringArm, USpringArmComponent::SocketName);
	ChaseCamera->FieldOfView = 90.0f;

	// ── Cockpit camera ────────────────────────────────────────────────────────
	CockpitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CockpitCamera"));
	CockpitCamera->SetupAttachment(ShipMesh);
	CockpitCamera->SetRelativeLocation(FVector(200.0f, 0.0f, 30.0f));
	CockpitCamera->FieldOfView = 110.0f;
	CockpitCamera->SetActive(false);

	// ── Engine particles ──────────────────────────────────────────────────────
	EngineTrailLeft = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EngineTrailLeft"));
	EngineTrailLeft->SetupAttachment(ShipMesh);
	EngineTrailLeft->SetRelativeLocation(FVector(-300.0f, -60.0f, 0.0f));

	EngineTrailRight = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EngineTrailRight"));
	EngineTrailRight->SetupAttachment(ShipMesh);
	EngineTrailRight->SetRelativeLocation(FVector(-300.0f, 60.0f, 0.0f));

	// ── Physics & effects components ──────────────────────────────────────────
	PhysicsComp = CreateDefaultSubobject<UBlackHolePhysicsComponent>(TEXT("PhysicsComp"));
	TidalForceComp = CreateDefaultSubobject<UTidalForceComponent>(TEXT("TidalForceComp"));
	TimeDilationComp = CreateDefaultSubobject<UTimeDilationComponent>(TEXT("TimeDilationComp"));
}

void ASpaceshipPawn::BeginPlay()
{
	Super::BeginPlay();

	// Find the nearest black hole
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlackHoleActor::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		NearestBlackHole = Cast<ABlackHoleActor>(FoundActors[0]);
		if (PhysicsComp)
			PhysicsComp->SetBlackHole(NearestBlackHole);
	}

	// Listen for ship destruction
	if (PhysicsComp)
	{
		PhysicsComp->OnShipDestroyed.AddDynamic(this, &ASpaceshipPawn::OnShipDestroyed);
	}

	SetCameraMode(ECameraMode::Chase);
}

void ASpaceshipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PhysicsComp) return;
	if (PhysicsComp->SpaghettificationState == ESpaghettificationState::Destroyed) return;

	// Apply player thrust
	ApplyPlayerThrust(DeltaTime);

	// Apply rotation inputs
	ApplyRotation(DeltaTime);

	// Combine velocities for total speed display
	FVector GravVel = PhysicsComp->GravityVelocity;
	TotalVelocity = PlayerVelocity + GravVel;
	float Speed = TotalVelocity.Size(); // m/s
	SpeedKmPerSec = Speed / 1000.0f;
	SpeedFractionOfC = Speed / static_cast<float>(C);

	// Update structural integrity
	StructuralIntegrity = PhysicsComp->StructuralIntegrity;

	// Engine visual feedback
	UpdateEngineParticles();
	UpdateHullDamageVisuals();
	CheckSpecialZones();
}

void ASpaceshipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Thrust axes
	PlayerInputComponent->BindAxis("ThrustForward",  this, &ASpaceshipPawn::OnThrustForward);
	PlayerInputComponent->BindAxis("ThrustStrafe",   this, &ASpaceshipPawn::OnThrustStrafe);
	PlayerInputComponent->BindAxis("ThrustVertical", this, &ASpaceshipPawn::OnThrustVertical);

	// Rotation axes
	PlayerInputComponent->BindAxis("Pitch", this, &ASpaceshipPawn::OnPitch);
	PlayerInputComponent->BindAxis("Yaw",   this, &ASpaceshipPawn::OnYaw);
	PlayerInputComponent->BindAxis("Roll",  this, &ASpaceshipPawn::OnRoll);

	// Actions
	PlayerInputComponent->BindAction("Boost",         IE_Pressed,  this, &ASpaceshipPawn::OnBoostPressed);
	PlayerInputComponent->BindAction("Boost",         IE_Released, this, &ASpaceshipPawn::OnBoostReleased);
	PlayerInputComponent->BindAction("Brake",         IE_Pressed,  this, &ASpaceshipPawn::OnBrakePressed);
	PlayerInputComponent->BindAction("Brake",         IE_Released, this, &ASpaceshipPawn::OnBrakeReleased);
	PlayerInputComponent->BindAction("ToggleCamera",  IE_Pressed,  this, &ASpaceshipPawn::ToggleCamera);
	PlayerInputComponent->BindAction("ToggleHUD",     IE_Pressed,  this, &ASpaceshipPawn::ToggleHUD);
}

// ─── Input Callbacks ──────────────────────────────────────────────────────────

void ASpaceshipPawn::OnThrustForward(float Value)  { ThrustAxis  = Value; }
void ASpaceshipPawn::OnThrustStrafe(float Value)   { StrafeAxis  = Value; }
void ASpaceshipPawn::OnThrustVertical(float Value) { VerticalAxis = Value; }
void ASpaceshipPawn::OnPitch(float Value)  { PitchAxis = Value; }
void ASpaceshipPawn::OnYaw(float Value)    { YawAxis   = Value; }
void ASpaceshipPawn::OnRoll(float Value)   { RollAxis  = Value; }

void ASpaceshipPawn::OnBoostPressed()  { bBoosting = true; }
void ASpaceshipPawn::OnBoostReleased() { bBoosting = false; }
void ASpaceshipPawn::OnBrakePressed()  { bBraking  = true; }
void ASpaceshipPawn::OnBrakeReleased() { bBraking  = false; }

// ─── Thrust ───────────────────────────────────────────────────────────────────

void ASpaceshipPawn::ApplyPlayerThrust(float DeltaTime)
{
	float EffectiveMass = ShipMassKg;
	float EffectiveThrust = ThrustForce * (bBoosting ? BoostMultiplier : 1.0f);

	// Acceleration = F/m
	float AccelMag = EffectiveThrust / EffectiveMass;

	FVector LocalThrust = FVector(ThrustAxis, StrafeAxis, VerticalAxis);
	bool bAnyInput = !LocalThrust.IsNearlyZero(0.01f);

	if (bAnyInput)
	{
		LocalThrust.Normalize();
		FVector WorldThrust = GetActorTransform().TransformVector(LocalThrust);
		PlayerVelocity += WorldThrust * AccelMag * DeltaTime;
	}

	// Emergency brake: rapidly damp player velocity
	if (bBraking)
	{
		PlayerVelocity *= FMath::Pow(1.0f - BrakeDamping, DeltaTime);
	}

	// Apply player velocity to actor position
	FVector NewPos = GetActorLocation() + PlayerVelocity * DeltaTime;
	SetActorLocation(NewPos, true);

	bEnginesFiring = bAnyInput;
}

// ─── Rotation ─────────────────────────────────────────────────────────────────

void ASpaceshipPawn::ApplyRotation(float DeltaTime)
{
	FRotator RotationDelta(
		PitchAxis * RotationSpeed * DeltaTime * MouseSensitivity,
		YawAxis   * RotationSpeed * DeltaTime * MouseSensitivity,
		RollAxis  * RotationSpeed * DeltaTime
	);

	AddActorLocalRotation(RotationDelta);
}

// ─── Camera ───────────────────────────────────────────────────────────────────

void ASpaceshipPawn::ToggleCamera()
{
	SetCameraMode(CameraMode == ECameraMode::Chase ? ECameraMode::Cockpit : ECameraMode::Chase);
}

void ASpaceshipPawn::SetCameraMode(ECameraMode NewMode)
{
	CameraMode = NewMode;
	bool bChase = (CameraMode == ECameraMode::Chase);
	ChaseCamera->SetActive(bChase);
	CockpitCamera->SetActive(!bChase);
}

void ASpaceshipPawn::ToggleHUD()
{
	bHUDVisible = !bHUDVisible;
}

// ─── Engine Particles ─────────────────────────────────────────────────────────

void ASpaceshipPawn::UpdateEngineParticles()
{
	if (EngineTrailLeft)
	{
		EngineTrailLeft->SetVisibility(bEnginesFiring);
		if (bEnginesFiring)
		{
			float Intensity = bBoosting ? 3.0f : 1.0f;
			EngineTrailLeft->SetFloatParameter(FName("ThrustIntensity"), Intensity);
		}
	}
	if (EngineTrailRight)
	{
		EngineTrailRight->SetVisibility(bEnginesFiring);
		if (bEnginesFiring)
		{
			float Intensity = bBoosting ? 3.0f : 1.0f;
			EngineTrailRight->SetFloatParameter(FName("ThrustIntensity"), Intensity);
		}
	}
}

// ─── Hull Damage Visuals ──────────────────────────────────────────────────────

void ASpaceshipPawn::UpdateHullDamageVisuals()
{
	// Broadcast when integrity crosses warning thresholds
	static const float Thresholds[] = { 0.75f, 0.50f, 0.25f, 0.10f };
	for (float Threshold : Thresholds)
	{
		if (StructuralIntegrity <= Threshold && LastIntegrityWarningLevel > Threshold)
		{
			OnHullDamageThreshold(StructuralIntegrity);
			LastIntegrityWarningLevel = Threshold;
		}
	}

	// Apply material damage parameter
	if (ShipMesh && StructuralIntegrity < 1.0f)
	{
		// Assumes material has a scalar param "DamageLevel"
		ShipMesh->SetScalarParameterValueOnMaterials(
			FName("DamageLevel"), 1.0f - StructuralIntegrity);
	}
}

// ─── Special Zone Checks ──────────────────────────────────────────────────────

void ASpaceshipPawn::CheckSpecialZones()
{
	if (!NearestBlackHole) return;

	FVector MyPos = GetActorLocation();
	double r = static_cast<double>((MyPos - NearestBlackHole->GetActorLocation()).Size());

	// Photon sphere warning (fired once)
	if (!bPhotonSphereWarningFired && r <= NearestBlackHole->PhotonSphereRadius)
	{
		OnPhotonSphereEntered();
		bPhotonSphereWarningFired = true;
	}

	// Approaching event horizon countdown
	double distToHorizon = NearestBlackHole->GetDistanceToEventHorizon(MyPos);
	if (distToHorizon < 1000.0e3 && distToHorizon > 0.0) // within 1000 km
	{
		OnEventHorizonApproach(static_cast<float>(distToHorizon / 1000.0));
	}
}
