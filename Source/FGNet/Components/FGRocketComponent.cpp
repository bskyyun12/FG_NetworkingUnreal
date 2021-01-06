// Fill out your copyright notice in the Description page of Project Settings.


#include "FGRocketComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UFGRocketComponent::UFGRocketComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(this);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshOb_torus(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube'"));
	if (StaticMeshOb_torus.Object)
	{
		MeshComponent->SetStaticMesh(StaticMeshOb_torus.Object);
	}

	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UFGRocketComponent::BeginPlay()
{
	Super::BeginPlay();

	// Owner will be the player that instantiated this
	CachedCollisionQueryParams.AddIgnoredActor(GetOwner());

	SetRocketVisibility(false);
}


// Called every frame
void UFGRocketComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	LifeTimeElapsed -= DeltaTime;
	DistanceMoved += MovementVelocity * DeltaTime;

	FacingRotationStart = FQuat::Slerp(FacingRotationStart.ToOrientationQuat(), FacingRotationCorrection, 0.9f * DeltaTime).Vector();

#if !UE_BUILD_SHIPPING
	if (bDebugDrawCorrection)
	{
		const float ArrowLength = 3000.0f;
		const float ArrowSize = 50.0f;
		DrawDebugDirectionalArrow(GetWorld(), RocketStartLocation, RocketStartLocation + OriginalFacingDirection * ArrowLength, ArrowSize, FColor::Red);
		DrawDebugDirectionalArrow(GetWorld(), RocketStartLocation, RocketStartLocation + FacingRotationStart * ArrowLength, ArrowSize, FColor::Green);
	}
#endif

	const FVector NewLocation = RocketStartLocation + FacingRotationStart * DistanceMoved;
	//SetWorldLocation(NewLocation);
	MeshComponent->SetWorldLocation(NewLocation);

	FHitResult Hit;
	const FVector StartLoc = NewLocation;
	const FVector EndLoc = StartLoc + FacingRotationStart * 100.0f;
	GetWorld()->LineTraceSingleByChannel(Hit, StartLoc, EndLoc, ECC_Visibility, CachedCollisionQueryParams);

	if (Hit.bBlockingHit)
	{
		Explode();
	}

	if (LifeTimeElapsed < 0.0f)
	{
		Explode();
	}
}

void UFGRocketComponent::SetRocketVisibility(bool bInVisible)
{
	SetVisibility(bInVisible, true);
}

void UFGRocketComponent::StartMoving(const FVector& Forward, const FVector& InStartLocation)
{
	FacingRotationStart = Forward;
	FacingRotationCorrection = FacingRotationStart.ToOrientationQuat();
	RocketStartLocation = InStartLocation;
	MeshComponent->SetWorldLocationAndRotation(InStartLocation, Forward.Rotation());
	bIsFree = false;
	SetComponentTickEnabled(true);
	SetRocketVisibility(true);
	LifeTimeElapsed = LifeTime;
	DistanceMoved = 0.0f;
	OriginalFacingDirection = FacingRotationStart;
}

void UFGRocketComponent::ApplyCorrection(const FVector& Forward)
{
	FacingRotationCorrection = Forward.ToOrientationQuat();
}

void UFGRocketComponent::Explode()
{
	if (Explosion != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Explosion, MeshComponent->GetComponentLocation(), MeshComponent->GetComponentRotation(), true);
	}

	MakeFree();
}

void UFGRocketComponent::MakeFree()
{
	bIsFree = true;
	SetComponentTickEnabled(false);
	SetRocketVisibility(false);
}

