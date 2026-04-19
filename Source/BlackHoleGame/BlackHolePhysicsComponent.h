// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlackHoleActor.h"
#include "BlackHolePhysicsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpaghettificationStateChanged, ESpaghettificationState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShipDestroyed);

/**
 * UBlackHolePhysicsComponent
 *
 * Handles all black hole physics interactions for the spaceship:
 *  - Gravitational acceleration each tick
 *  - Frame dragging (Lense-Thirring effect)
 *  - Orbital state detection
 *  - Spaghettification state machine
 *
 * Attach to SpaceshipPawn.
 */
UCLASS(ClassGroup=(BlackHole), meta=(BlueprintSpawnableComponent),
	DisplayName="Black Hole Physics Component")
class BLACKHOLEGAME_API UBlackHolePhysicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBlackHolePhysicsComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ────────────────────────────────────────────────────────

	/** Approximate length of the spaceship for tidal force calculation (meters) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float ShipLength_m = 50.0f;

	/** Whether gravity is currently enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	bool bGravityEnabled = true;

	/** Whether frame dragging is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	bool bFrameDraggingEnabled = true;

	/** Tidal stress threshold in m/s² above which we enter STRESSED state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Tidal")
	float TidalStressThreshold = 10.0f;

	/** Multiplier relative to threshold at which we start STRETCHING */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Tidal")
	float StretchingMultiplier = 3.0f;

	/** Multiplier at which structural FAILURE occurs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Tidal")
	float CriticalMultiplier = 1.0f;

	// ── Runtime State ─────────────────────────────────────────────────────────

	/** Current orbital state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics|Orbital")
	EOrbitalState OrbitalState = EOrbitalState::Escape;

	/** Current spaghettification state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics|Tidal")
	ESpaghettificationState SpaghettificationState = ESpaghettificationState::Normal;

	/** Current tidal acceleration magnitude (m/s²) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics|Tidal")
	float CurrentTidalAcceleration = 0.0f;

	/** Current gravitational acceleration magnitude (m/s²) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	float CurrentGravAcceleration = 0.0f;

	/** Current gravitational acceleration in units of g (9.81 m/s²) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	float GravAccelInG = 0.0f;

	/** Accumulated velocity from gravity (UU/s = m/s) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	FVector GravityVelocity = FVector::ZeroVector;

	/** Stretch scale applied along radial axis due to tidal forces */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics|Tidal")
	FVector TidalStretchScale = FVector(1.0f, 1.0f, 1.0f);

	/** Current structural integrity 0..1 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics|Tidal")
	float StructuralIntegrity = 1.0f;

	// ── Delegates ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Physics|Events")
	FOnSpaghettificationStateChanged OnSpaghettificationStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Physics|Events")
	FOnShipDestroyed OnShipDestroyed;

	// ── Accessors ─────────────────────────────────────────────────────────────

	/** Set the black hole reference (called by GameMode or SpaceshipPawn on spawn) */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void SetBlackHole(ABlackHoleActor* InBlackHole) { BlackHole = InBlackHole; }

	UFUNCTION(BlueprintCallable, Category = "Physics")
	ABlackHoleActor* GetBlackHole() const { return BlackHole; }

private:
	UPROPERTY()
	TObjectPtr<ABlackHoleActor> BlackHole = nullptr;

	void UpdateOrbitalState();
	void UpdateSpaghettificationState(float DeltaTime);
	void UpdateTidalStretch();
	ESpaghettificationState ComputeNewSpaghettificationState() const;
};
