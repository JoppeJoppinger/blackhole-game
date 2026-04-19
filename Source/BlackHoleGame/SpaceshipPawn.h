// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "BlackHolePhysicsComponent.h"
#include "TidalForceComponent.h"
#include "TimeDilationComponent.h"
#include "SpaceshipPawn.generated.h"

// Camera mode enum
UENUM(BlueprintType)
enum class ECameraMode : uint8
{
	Cockpit    UMETA(DisplayName = "Cockpit (1st Person)"),
	Chase      UMETA(DisplayName = "Chase (3rd Person)")
};

/**
 * ASpaceshipPawn
 *
 * 6-degrees-of-freedom spaceship controlled by the player.
 * Integrates with BlackHolePhysicsComponent for gravitational effects.
 * Handles input, camera switching, mesh deformation, HUD data.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(BlackHole))
class BLACKHOLEGAME_API ASpaceshipPawn : public APawn
{
	GENERATED_BODY()

public:
	ASpaceshipPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ── Components ────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UStaticMeshComponent> ShipMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<USpringArmComponent> ChaseSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UCameraComponent> CockpitCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UNiagaraComponent> EngineTrailLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UNiagaraComponent> EngineTrailRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UBlackHolePhysicsComponent> PhysicsComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UTidalForceComponent> TidalForceComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|Components")
	TObjectPtr<UTimeDilationComponent> TimeDilationComp;

	// ── Movement Settings ─────────────────────────────────────────────────────

	/** Maximum player-applied thrust force (Newtons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spaceship|Movement")
	float ThrustForce = 5.0e6f;

	/** Boost multiplier when Shift is held */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spaceship|Movement")
	float BoostMultiplier = 5.0f;

	/** Rotation speed in degrees/second from keyboard */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spaceship|Movement")
	float RotationSpeed = 60.0f;

	/** Mouse look sensitivity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spaceship|Movement")
	float MouseSensitivity = 0.5f;

	/** Ship mass in kg (used with ThrustForce to compute acceleration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spaceship|Movement")
	float ShipMassKg = 100000.0f;

	/** Emergency brake deceleration coefficient (fraction of velocity removed per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spaceship|Movement")
	float BrakeDamping = 0.85f;

	// ── Runtime State ─────────────────────────────────────────────────────────

	/** Player-applied velocity (separate from gravity velocity) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	FVector PlayerVelocity = FVector::ZeroVector;

	/** Total velocity (player + gravity) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	FVector TotalVelocity = FVector::ZeroVector;

	/** Speed in km/s */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	float SpeedKmPerSec = 0.0f;

	/** Speed as fraction of c */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	float SpeedFractionOfC = 0.0f;

	/** Structural integrity 0..1 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	float StructuralIntegrity = 1.0f;

	/** Current camera mode */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	ECameraMode CameraMode = ECameraMode::Chase;

	/** Whether the HUD is visible */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	bool bHUDVisible = true;

	/** Whether engines are firing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spaceship|State")
	bool bEnginesFiring = false;

	// ── Blueprint Events ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintImplementableEvent, Category = "Spaceship|Events")
	void OnHullDamageThreshold(float NewIntegrity);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spaceship|Events")
	void OnShipDestroyed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Spaceship|Events")
	void OnPhotonSphereEntered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Spaceship|Events")
	void OnEventHorizonApproach(float DistanceKm);

	// ── Camera ────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Spaceship|Camera")
	void ToggleCamera();

	UFUNCTION(BlueprintCallable, Category = "Spaceship|Camera")
	void SetCameraMode(ECameraMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Spaceship")
	void ToggleHUD();

private:
	// Input axis state
	float ThrustAxis = 0.0f;
	float StrafeAxis = 0.0f;
	float VerticalAxis = 0.0f;
	float PitchAxis = 0.0f;
	float YawAxis = 0.0f;
	float RollAxis = 0.0f;
	bool bBoosting = false;
	bool bBraking = false;

	// Input callbacks
	void OnThrustForward(float Value);
	void OnThrustStrafe(float Value);
	void OnThrustVertical(float Value);
	void OnPitch(float Value);
	void OnYaw(float Value);
	void OnRoll(float Value);
	void OnBoostPressed();
	void OnBoostReleased();
	void OnBrakePressed();
	void OnBrakeReleased();

	void ApplyPlayerThrust(float DeltaTime);
	void ApplyRotation(float DeltaTime);
	void UpdateEngineParticles();
	void UpdateHullDamageVisuals();
	void CheckSpecialZones();

	float LastIntegrityWarningLevel = 1.0f;

	UPROPERTY()
	TObjectPtr<class ABlackHoleActor> NearestBlackHole;

	bool bPhotonSphereWarningFired = false;
};
