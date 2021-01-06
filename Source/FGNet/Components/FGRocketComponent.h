// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FGRocketComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FGNET_API UFGRocketComponent : public USceneComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere)
	float MovementVelocity = 1300.0f;

	UPROPERTY(EditAnywhere)
	float LifeTime = 2.0f;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystem* Explosion = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* MeshComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = Debug)
	bool bDebugDrawCorrection = true;

	FVector OriginalFacingDirection = FVector::ZeroVector;
	FVector FacingRotationStart = FVector::ZeroVector;
	FQuat FacingRotationCorrection = FQuat::Identity;
	FVector RocketStartLocation = FVector::ZeroVector;

	FCollisionQueryParams CachedCollisionQueryParams;

	bool bIsFree = true;
	float LifeTimeElapsed = 0.0f;
	float DistanceMoved = 0.0f;

private:
	void SetRocketVisibility(bool bInVisible);

public:
	UFGRocketComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartMoving(const FVector& Forward, const FVector& InStartLocation);
	void ApplyCorrection(const FVector& Forward);

	bool IsFree() const { return bIsFree; }

	void Explode();
	void MakeFree();
};
