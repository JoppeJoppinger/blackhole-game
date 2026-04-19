// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "BlackHoleActor.generated.h"

// ─── Physical Constants ───────────────────────────────────────────────────────
namespace BlackHolePhysics
{
	// Gravitational constant (SI)
	static constexpr double G = 6.674e-11;
	// Speed of light (m/s)
	static constexpr double C = 299792458.0;
	static constexpr double C2 = C * C;
	// One solar mass in kg
	static constexpr double SolarMass = 1.989e30;
	// Standard gravity
	static constexpr double StandardGravity = 9.80665;
	// 1 Unreal Unit = 1 meter
	static constexpr double UU_TO_METER = 1.0;
}

// ─── Spaghettification State ──────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ESpaghettificationState : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	Stressed    UMETA(DisplayName = "Stressed"),
	Stretching  UMETA(DisplayName = "Stretching"),
	Critical    UMETA(DisplayName = "Critical"),
	Destroyed   UMETA(DisplayName = "Destroyed")
};

// ─── Orbital State ────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EOrbitalState : uint8
{
	Escape      UMETA(DisplayName = "Escape Trajectory"),
	Elliptical  UMETA(DisplayName = "Elliptical Orbit"),
	Circular    UMETA(DisplayName = "Circular Orbit"),
	Captured    UMETA(DisplayName = "Captured — Falling In")
};

/**
 * ABlackHoleActor
 *
 * Represents a rotating Kerr black hole with accurate relativistic physics.
 * Mass defaults to 10 solar masses, giving a Schwarzschild radius of ~29.5 km.
 *
 * All internal math uses SI units (meters, kg, seconds).
 * Positions passed in and returned are in Unreal Units (= meters per 1 UU = 1 m convention).
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(BlackHole), meta=(DisplayName="Black Hole Actor"))
class BLACKHOLEGAME_API ABlackHoleActor : public AActor
{
	GENERATED_BODY()

public:
	ABlackHoleActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ── Configuration ────────────────────────────────────────────────────────

	/** Black hole mass in solar masses (default 10 = ~29.5 km Schwarzschild radius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole|Physics",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	double MassInSolarMasses = 10.0;

	/** Kerr spin parameter a = J/(Mc), dimensionless 0..1 (0 = Schwarzschild, 1 = extremal Kerr) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole|Physics",
		meta = (ClampMin = "0.0", ClampMax = "0.99"))
	double SpinParameter = 0.7;

	/** Rotation axis of the black hole in world space (normalised automatically) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole|Physics")
	FVector SpinAxis = FVector(0.0, 0.0, 1.0);

	// ── Computed Properties (read-only in editor) ─────────────────────────────

	/** Schwarzschild radius in meters (== UU) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Black Hole|Computed")
	double SchwarzschildRadius = 0.0;

	/** ISCO radius for prograde orbit (meters) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Black Hole|Computed")
	double ISCORadius = 0.0;

	/** Photon sphere radius (meters) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Black Hole|Computed")
	double PhotonSphereRadius = 0.0;

	/** Black hole mass in kg */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Black Hole|Computed")
	double MassKg = 0.0;

	// ── Visualisation ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Black Hole|Visual")
	TObjectPtr<USphereComponent> EventHorizonSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Black Hole|Visual")
	TObjectPtr<USphereComponent> PhotonSphereBoundary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Black Hole|Visual")
	TObjectPtr<USphereComponent> ISCOBoundary;

	// ── Physics Queries ───────────────────────────────────────────────────────

	/**
	 * Returns the Newtonian gravitational acceleration vector (in UU/s²) at WorldPosition.
	 * Direction points toward the black hole centre.
	 */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	FVector GetGravitationalAcceleration(FVector WorldPosition) const;

	/**
	 * Returns the tidal (differential gravity) acceleration magnitude experienced by an
	 * object of length ObjectLength_m (meters) at WorldPosition.
	 * Formula: a_tidal = 2GM * l / r³
	 * Returned as a vector along the radial axis scaled by the tidal magnitude.
	 */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	FVector GetTidalAcceleration(FVector WorldPosition, float ObjectLength_m) const;

	/**
	 * Returns the gravitational time dilation factor at WorldPosition.
	 * Factor = sqrt(1 - rs/r).  Returns 0 at the horizon, 1 at infinity.
	 * Result is clamped to [0, 1].
	 */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	double GetTimeDilationFactor(FVector WorldPosition) const;

	/**
	 * Returns the frame-dragging (Lense-Thirring) force vector for an object at WorldPosition
	 * moving with Velocity (UU/s).  Uses linearised Kerr approximation valid far from horizon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	FVector GetFrameDraggingForce(FVector WorldPosition, FVector Velocity) const;

	/**
	 * Returns the escape velocity in m/s at WorldPosition.
	 * Formula: v_escape = sqrt(2GM/r)
	 */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	double GetEscapeVelocity(FVector WorldPosition) const;

	/**
	 * Returns the gravitational redshift parameter z = 1/sqrt(1-rs/r) - 1.
	 * z > 0 means redshifted (light climbing out), z → ∞ at the horizon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	double GetGravitationalRedshift(FVector WorldPosition) const;

	/**
	 * Distance from WorldPosition to the event horizon surface in meters.
	 * Negative if inside the horizon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	double GetDistanceToEventHorizon(FVector WorldPosition) const;

	/** Returns true if WorldPosition is inside the event horizon. */
	UFUNCTION(BlueprintCallable, Category = "Black Hole|Physics")
	bool IsInsideEventHorizon(FVector WorldPosition) const;

private:
	void ComputeDerivedConstants();
	void ApplyGravityToPhysicsActors(float DeltaTime);

	// Cache frequently-used GM product
	double GM = 0.0;
};
