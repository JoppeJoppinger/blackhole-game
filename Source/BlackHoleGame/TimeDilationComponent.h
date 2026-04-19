// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeDilationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeDilationChanged,
	double, NewDilationFactor, double, ProperTimeDelta);

/**
 * UTimeDilationComponent
 *
 * Tracks the proper time (ship clock) vs coordinate time (universal clock).
 * Uses gravitational time dilation:  dt_proper = dt_coord * sqrt(1 - rs/r)
 *
 * In the full GR picture, velocity also dilates time (special relativistic).
 * This component includes both gravitational and kinematic dilation:
 *   dt_proper = dt_coord * sqrt(1 - rs/r) * sqrt(1 - v²/c²)
 *
 * The HUD reads ProperTimeAccumulated and CoordinateTimeAccumulated.
 */
UCLASS(ClassGroup=(BlackHole), meta=(BlueprintSpawnableComponent),
	DisplayName="Time Dilation Component")
class BLACKHOLEGAME_API UTimeDilationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTimeDilationComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ── Runtime State ─────────────────────────────────────────────────────────

	/** Current gravitational dilation factor sqrt(1 - rs/r). 0=horizon, 1=far. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Dilation")
	double GravitationalDilationFactor = 1.0;

	/** Current kinematic dilation factor sqrt(1 - v²/c²). 1=at rest, 0=at c. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Dilation")
	double KinematicDilationFactor = 1.0;

	/** Combined dilation factor (gravitational * kinematic) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Dilation")
	double TotalDilationFactor = 1.0;

	/** Accumulated proper time on ship clock (seconds in-game) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Dilation")
	double ProperTimeAccumulated = 0.0;

	/** Accumulated coordinate time (universal clock, seconds) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Dilation")
	double CoordinateTimeAccumulated = 0.0;

	/** Current ship speed as fraction of c (0..1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Dilation")
	double SpeedFractionOfC = 0.0;

	/** Current ship speed in km/s */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Dilation")
	double SpeedKmPerSecond = 0.0;

	/** Threshold for broadcasting dilation change event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Dilation")
	double DilationChangeBroadcastThreshold = 0.005;

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Time Dilation|Events")
	FOnTimeDilationChanged OnTimeDilationChanged;

	// ── Manual Override ───────────────────────────────────────────────────────

	/** Reset both clocks to zero */
	UFUNCTION(BlueprintCallable, Category = "Time Dilation")
	void ResetClocks();

private:
	double LastBroadcastDilationFactor = 1.0;
};
