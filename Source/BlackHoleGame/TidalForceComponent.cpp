// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "TidalForceComponent.h"
#include "BlackHolePhysicsComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

UTidalForceComponent::UTidalForceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTidalForceComponent::BeginPlay()
{
	Super::BeginPlay();
	FindMeshComponent();
}

void UTidalForceComponent::FindMeshComponent()
{
	if (!GetOwner()) return;

	if (MeshComponentName != NAME_None)
	{
		TArray<USceneComponent*> Components;
		GetOwner()->GetComponents<USceneComponent>(Components);
		for (USceneComponent* Comp : Components)
		{
			if (Comp && Comp->GetFName() == MeshComponentName)
			{
				TargetMeshComponent = Comp;
				return;
			}
		}
	}

	// Fallback: use root scene component
	TargetMeshComponent = GetOwner()->GetRootComponent();
}

void UTidalForceComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner()) return;

	// Get target stretch scale from BlackHolePhysicsComponent
	UBlackHolePhysicsComponent* PhysComp =
		GetOwner()->FindComponentByClass<UBlackHolePhysicsComponent>();

	FVector TargetScale = FVector::OneVector;
	float Integrity = 1.0f;

	if (PhysComp)
	{
		TargetScale = PhysComp->TidalStretchScale;
		TidalAccelMagnitude = PhysComp->CurrentTidalAcceleration;
		Integrity = PhysComp->StructuralIntegrity;

		// Clamp max stretch
		TargetScale.X = FMath::Clamp(TargetScale.X, 0.01f, MaxStretchRatio);
		TargetScale.Y = FMath::Clamp(TargetScale.Y, 0.01f, MaxStretchRatio);
		TargetScale.Z = FMath::Clamp(TargetScale.Z, 0.01f, MaxStretchRatio);
	}

	ApplyDeformationToMesh(TargetScale, DeltaTime);
	OnTidalForceUpdated.Broadcast(TidalAccelMagnitude, Integrity);
}

void UTidalForceComponent::ApplyDeformationToMesh(const FVector& TargetScale, float DeltaTime)
{
	// Smooth interpolation toward target scale
	CurrentDeformScale = FMath::VInterpTo(
		CurrentDeformScale, TargetScale, DeltaTime, DeformationInterpSpeed);

	if (TargetMeshComponent)
	{
		TargetMeshComponent->SetRelativeScale3D(CurrentDeformScale);
	}
}
