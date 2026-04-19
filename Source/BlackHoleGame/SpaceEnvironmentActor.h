// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "SpaceEnvironmentActor.generated.h"

/** A single star in the procedural starfield */
USTRUCT(BlueprintType)
struct FStarData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FVector Direction = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FLinearColor Color = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) float Magnitude = 1.0f;
	UPROPERTY(BlueprintReadOnly) uint8 SpectralClass = 4; // 0=O, 1=B, 2=A, 3=F, 4=G, 5=K, 6=M
};

/**
 * ASpaceEnvironmentActor
 *
 * Generates a procedural starfield with 10,000+ stars using realistic
 * spectral type distributions and colors.  Also manages relativistic aberration
 * based on ship velocity.
 *
 * Stars are rendered as instanced static mesh points on a large skybox sphere.
 * The Milky Way band, distant nebulae, and cosmic microwave background are
 * handled via material parameters on the skybox mesh.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(BlackHole),
	meta=(DisplayName="Space Environment Actor"))
class BLACKHOLEGAME_API ASpaceEnvironmentActor : public AActor
{
	GENERATED_BODY()

public:
	ASpaceEnvironmentActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ── Configuration ────────────────────────────────────────────────────────

	/** Total number of procedural stars to generate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Stars",
		meta = (ClampMin = "1000", ClampMax = "100000"))
	int32 NumStars = 12000;

	/** Radius of the skybox sphere in UU (meters).  Should be >> ship scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Stars")
	float SkyboxRadius = 1.0e10f; // 10 million km

	/** Seed for random star generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Stars")
	int32 RandomSeed = 42;

	/** Whether relativistic aberration is applied each tick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Aberration")
	bool bRelativisticAberration = true;

	/** Name tag of the spaceship pawn to query for velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|Aberration")
	TSubclassOf<APawn> SpaceshipClass;

	// ── Runtime State ─────────────────────────────────────────────────────────

	/** Generated star data (filled at BeginPlay) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment|Stars")
	TArray<FStarData> Stars;

	/** Current ship beta (v/c) used for aberration */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment|Aberration")
	float CurrentBeta = 0.0f;

	// ── Components ────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
	TObjectPtr<UStaticMeshComponent> SkyboxMesh;

	// ── Utilities ─────────────────────────────────────────────────────────────

	/** Returns the spectral color for a given spectral class index (0=O..6=M) */
	UFUNCTION(BlueprintCallable, Category = "Environment|Stars")
	static FLinearColor GetSpectralColor(uint8 SpectralClass);

	/**
	 * Applies relativistic aberration to a direction vector.
	 * formula: cos(theta') = (cos(theta) - beta) / (1 - beta*cos(theta))
	 * @param Direction  Original star direction (unit vector)
	 * @param ShipVelocity  Ship velocity vector (m/s)
	 * @return Apparent direction after aberration
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment|Aberration")
	static FVector ApplyRelativisticAberration(FVector Direction, FVector ShipVelocity);

private:
	void GenerateStarField();
	void UpdateAberration();

	// Cache
	UPROPERTY()
	TObjectPtr<APawn> CachedShip;
};
