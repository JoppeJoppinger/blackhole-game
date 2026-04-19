// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TidalForceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTidalForceUpdated, float, TidalMagnitude, float, StructuralIntegrity);

/**
 * UTidalForceComponent
 *
 * Dedicated component that tracks tidal (spaghettification) forces independently
 * of the physics integration.  Applies mesh deformation scale to an attached
 * static/skeletal mesh component, and exposes damage events.
 *
 * Works together with UBlackHolePhysicsComponent.
 */
UCLASS(ClassGroup=(BlackHole), meta=(BlueprintSpawnableComponent),
	DisplayName="Tidal Force Component")
class BLACKHOLEGAME_API UTidalForceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTidalForceComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ────────────────────────────────────────────────────────

	/** Name of the mesh component to deform (must be on the same actor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tidal|Deformation")
	FName MeshComponentName = NAME_None;

	/** How fast the stretch scale interpolates toward the target (per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tidal|Deformation",
		meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float DeformationInterpSpeed = 3.0f;

	/** Maximum stretch ratio allowed (prevents insane geometry) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tidal|Deformation",
		meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float MaxStretchRatio = 8.0f;

	// ── Runtime State ─────────────────────────────────────────────────────────

	/** Current smoothed stretch scale applied to mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tidal")
	FVector CurrentDeformScale = FVector::OneVector;

	/** Raw tidal acceleration magnitude this frame (m/s²) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tidal")
	float TidalAccelMagnitude = 0.0f;

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Tidal|Events")
	FOnTidalForceUpdated OnTidalForceUpdated;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> TargetMeshComponent = nullptr;

	void FindMeshComponent();
	void ApplyDeformationToMesh(const FVector& TargetScale, float DeltaTime);
};
