#include "FGPlayer.h"
#include "../FGMovementStatics.h"
#include "../Components/FGMovementComponent.h"

#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Engine/NetDriver.h"
#include "Net\UnrealNetwork.h"

AFGPlayer::AFGPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	MeshOffsetRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshOffset"));
	MeshOffsetRoot->SetupAttachment(CollisionComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(MeshOffsetRoot);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->SetupAttachment(CollisionComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	MovementComponent = CreateDefaultSubobject<UFGMovementComponent>(TEXT("MovementComponent"));

	SetReplicateMovement(false);
}

void AFGPlayer::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent->SetUpdatedComponent(CollisionComponent);

	if (HasAuthority())
	{
		NetUpdateFrequency = NetFrequency;
	}
}

void AFGPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFGPlayer, ServerState);
}

void AFGPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_AutonomousProxy || (GetLocalRole() == ROLE_Authority && IsLocallyControlled()))
	{
		FPlayerMove Move;
		Move.Forward = Forward;
		Move.Turn = Turn;
		Move.DeltaTime = DeltaTime;
		Move.Time = GetWorld()->TimeSeconds;

		LastMove = Move;
		SimulateMove(LastMove);
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)	// Player
	{
		Server_SendMove(LastMove);	// Simulate Move + Update Server State
	}

	if (GetLocalRole() == ROLE_Authority && IsLocallyControlled())	// Server
	{
		UpdateServerState();
	}

	if (GetLocalRole() == ROLE_SimulatedProxy)	// Other Players
	{
		TimeSinceLastUpdate += DeltaTime;

		if (TimeBetweenUpdates < 0.01f || TimeSinceLastUpdate < 0.01f)
		{
			return;
		}

		float Alpha = TimeSinceLastUpdate / TimeBetweenUpdates;

		FVector StartLocation = ClientStartTransform.GetLocation();
		FVector TargetLocation = ServerState.Transform.GetLocation();
		FVector NewLocation = FMath::LerpStable(StartLocation, TargetLocation, Alpha);
		SetActorLocation(NewLocation);

		//FVector StartLocation = ClientStartTransform.GetLocation();
		//FVector TargetLocation = ServerState.Transform.GetLocation();
		//float VelocityToDerivative = ClientTimeBetweenLastUpdates * 100; // x100 to convert meter to centimeter
		//FVector StartDerivative = ClientStartVelocity * VelocityToDerivative;
		//FVector TargetDerivative = ServerState.VelocityVector * VelocityToDerivative;
		//FVector NewLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, Alpha);
		//SetActorLocation(NewLocation);

		//FVector NewDerivative = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, Alpha);
		//FVector NewVelocity = NewDerivative / VelocityToDerivative;
		//MovementVelocity = NewVelocity.Size();

		//FVector TargetLocation = ServerState.Transform.GetLocation();
		//FVector NewLocation = FMath::VInterpTo(GetActorLocation(), ServerState.Transform.GetLocation(), DeltaTime, LocationInterpSpeed);
		//SetActorLocation(NewLocation);

		FRotator TargetRotation = ServerState.Transform.GetRotation().Rotator();
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, RotationInterpSpeed);
		SetActorRotation(NewRotation);
	}
}

void AFGPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Accelerate"), this, &AFGPlayer::Handle_Accelerate);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFGPlayer::Handle_Turn);

	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &AFGPlayer::Handle_BrakePressed);
	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &AFGPlayer::Handle_BrakeReleased);
}

int32 AFGPlayer::GetPing() const
{
	if (GetPlayerState())
	{
		return static_cast<int32>(GetPlayerState()->GetPing());
	}

	return 0;
}

void AFGPlayer::OnRep_ServerState()
{
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		SetActorTransform(ServerState.Transform);
	}

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		TimeBetweenUpdates = TimeSinceLastUpdate;
		TimeSinceLastUpdate = 0;

		ClientStartVelocity = MovementComponent->GetFacingDirection() * MovementVelocity;
		ClientStartTransform = GetActorTransform();
	}
}

void AFGPlayer::UpdateServerState()
{
	ServerState.LastMove = LastMove;
	ServerState.Transform = GetActorTransform();
	//ServerState.VelocityVector = MovementComponent->GetFacingDirection() * MovementVelocity;
}

void AFGPlayer::Server_SendMove_Implementation(const FPlayerMove& Move)
{
	SimulateMove(Move);
	UpdateServerState();

	//Mulitcast_SendMove(Move);
}

void AFGPlayer::SimulateMove(const FPlayerMove& Move)
{
	const float Friction = IsBraking() ? BrakingFriction : DefaultFriction;
	const float Alpha = FMath::Clamp(FMath::Abs(MovementVelocity / (MaxVelocity * 0.75F)), 0.0F, 1.0F);
	const float TurnSpeed = FMath::InterpEaseOut(0.0F, TurnSpeedDefault, Alpha, 5.0F);
	const float MovementDirection = MovementVelocity > 0.0F ? Move.Turn : -Move.Turn;

	Yaw += (MovementDirection * TurnSpeed) * Move.DeltaTime;
	FQuat WantedFacingDirection = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
	MovementComponent->SetFacingRotation(WantedFacingDirection);

	FFGFrameMovement FrameMovement = MovementComponent->CreateFrameMovement();

	MovementVelocity += Move.Forward * Acceleration * Move.DeltaTime;
	MovementVelocity = FMath::Clamp(MovementVelocity, -MaxVelocity, MaxVelocity);
	MovementVelocity *= FMath::Pow(Friction, Move.DeltaTime);

	MovementComponent->ApplyGravity();
	FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * Move.DeltaTime);
	MovementComponent->Move(FrameMovement);
}

void AFGPlayer::Handle_Accelerate(float Value)
{
	Forward = Value;
}

void AFGPlayer::Handle_Turn(float Value)
{
	Turn = Value;
}

void AFGPlayer::Handle_BrakePressed()
{
	bIsBraking = true;
}

void AFGPlayer::Handle_BrakeReleased()
{
	bIsBraking = false;
}
