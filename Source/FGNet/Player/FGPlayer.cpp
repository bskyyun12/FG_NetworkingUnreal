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
#include "Net/UnrealNetwork.h"
#include "PlayerSetting.h"
#include "../DebugWidget.h"
#include "../Pickup.h"
#include "../FGRocket.h"
#include "../Components/FGRocketComponent.h"

AFGPlayer::AFGPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->SetupAttachment(CollisionComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	MovementComponent = CreateDefaultSubobject<UFGMovementComponent>(TEXT("MovementComponent"));

	SetReplicateMovement(false);

	if (HasAuthority())
	{
		USceneComponent* Rockets = CreateDefaultSubobject<USceneComponent>(TEXT("Rockets"));
		Rockets->SetupAttachment(CollisionComponent);

		const int32 RocketCache = 8;
		for (int32 Index = 0; Index < RocketCache; ++Index)
		{
			UFGRocketComponent* RocketComp = CreateDefaultSubobject<UFGRocketComponent>(*FString("RocketComp" + FString::FromInt(Index)));
			RocketComp->SetupAttachment(Rockets);
			RocketInstances.Add(RocketComp);
		}
	}
}

void AFGPlayer::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent->SetUpdatedComponent(CollisionComponent);

	CreateDebugWidget();
	if (DebugMenuInstance != nullptr)
	{
		DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	//SpawnRockets();
	BP_OnNumRocketsChanged(NumRockets);
	BP_OnHealthChanged(Health);

	OriginalMeshOffset = MeshComponent->GetRelativeLocation();
}

void AFGPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FireCooldownElapsed -= DeltaTime;

	FFGFrameMovement FrameMovement = MovementComponent->CreateFrameMovement();

	if (IsLocallyControlled())
	{
		ClientTimeStamp += DeltaTime;

		if (PlayerSetting == nullptr)
		{
			return;
		}

		const float MaxVelocity = PlayerSetting->MaxVelocity;
		const float Friction = IsBraking() ? PlayerSetting->BrakingFriction : PlayerSetting->Friction;
		const float Alpha = FMath::Clamp(FMath::Abs(MovementVelocity / (MaxVelocity * 0.75F)), 0.0F, 1.0F);
		const float TurnSpeed = FMath::InterpEaseOut(0.0F, PlayerSetting->TurnSpeedDefault, Alpha, 5.0F);
		const float TurnDirection = MovementVelocity > 0.0F ? Turn : -Turn;

		Yaw += (TurnDirection * TurnSpeed) * DeltaTime;
		FQuat WantedFacingDirection = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
		MovementComponent->SetFacingRotation(WantedFacingDirection);

		//MovementVelocity += Forward * PlayerSetting->Acceleration * DeltaTime;
		//MovementVelocity = FMath::Clamp(MovementVelocity, -PlayerSetting->MaxVelocity, PlayerSetting->MaxVelocity);
		AddMovementVelocity(DeltaTime);
		MovementVelocity *= FMath::Pow(Friction, DeltaTime);

		MovementComponent->ApplyGravity();
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);
		MovementComponent->Move(FrameMovement);

		//Server_SendTransform(GetActorTransform());
		//Server_SendYaw(MovementComponent->GetFacingRotation().Yaw);
		Server_SendMovement(GetActorLocation(), ClientTimeStamp, Forward, GetActorRotation().Yaw);
	}
	else
	{
		const float Friction = IsBraking() ? PlayerSetting->BrakingFriction : PlayerSetting->Friction;
		MovementVelocity *= FMath::Pow(Friction, DeltaTime);
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);
		MovementComponent->Move(FrameMovement);

		if (bPerformNetworkSmoothing)
		{
			const FVector NewRelativeLocation = FMath::VInterpTo(MeshComponent->GetRelativeLocation(), OriginalMeshOffset, LastCorrectionDelta, 1.75f);
			SetActorLocation(NewRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
		}

		//FVector NewLocation = FMath::VInterpTo(GetActorLocation(), ReplicatedLocation, DeltaTime, LocationLerpSpeed);
		//SetActorLocation(NewLocation);

		////FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation.Rotator(), DeltaTime, RotationLerpSpeed);
		////SetActorRotation(NewRotation);
		//MovementComponent->SetFacingRotation(FRotator(0.0f, ReplicatedYaw, 0.0f));
		//SetActorRotation(MovementComponent->GetFacingRotation());
	}
}

void AFGPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Accelerate"), this, &AFGPlayer::Handle_Accelerate);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFGPlayer::Handle_Turn);

	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &AFGPlayer::Handle_BrakePressed);
	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &AFGPlayer::Handle_BrakeReleased);

	PlayerInputComponent->BindAction(TEXT("DebugMenu"), IE_Pressed, this, &AFGPlayer::Handle_DebugMenuPressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AFGPlayer::Handle_FirePressed);
}

int32 AFGPlayer::GetPing() const
{
	if (GetPlayerState())
	{
		return static_cast<int32>(GetPlayerState()->GetPing());
	}

	return 0;
}

void AFGPlayer::Server_SendTransform_Implementation(const FTransform& TransformToSend)
{
	//Mulitcast_SendTransform(TransformToSend);
	ReplicatedLocation = TransformToSend.GetLocation();
}

void AFGPlayer::Mulitcast_SendTransform_Implementation(const FTransform& TransformToSend)
{
	if (!IsLocallyControlled())
	{
		TargetLocation = TransformToSend.GetLocation();
		TargetRotation = TransformToSend.GetRotation();
	}
}

const static float MaxMoveDeltaTime = 0.125f;

void AFGPlayer::Server_SendMovement_Implementation(const FVector& ClientLocation, float TimeStamp, float ClientForward, float ClientYaw)
{
	Multicast_SendMovement(ClientLocation, TimeStamp, ClientForward, ClientYaw);
}

void AFGPlayer::Multicast_SendMovement_Implementation(const FVector& InClientLocation, float TimeStamp, float ClientForward, float ClientYaw)
{
	if (!IsLocallyControlled())
	{
		Forward = ClientForward;
		const float DeltaTime = FMath::Min(TimeStamp - ClientTimeStamp, MaxMoveDeltaTime);
		ClientTimeStamp = TimeStamp;

		AddMovementVelocity(DeltaTime);
		MovementComponent->SetFacingRotation(FRotator(0.0f, ClientYaw, 0.0f));

		const FVector DeltaDiff = InClientLocation - GetActorLocation();

		if (DeltaDiff.SizeSquared() > FMath::Square(40.0f))
		{
			if (bPerformNetworkSmoothing)
			{
				const FScopedPreventAttachedComponentMove PreventMeshMove(MeshComponent);
				MovementComponent->UpdatedComponent->SetWorldLocation(InClientLocation, false, nullptr, ETeleportType::TeleportPhysics);
				LastCorrectionDelta = DeltaTime;
			}
			else
			{
				SetActorLocation(InClientLocation);
			}
		}
	}
}

void AFGPlayer::AddMovementVelocity(float DeltaTime)
{
	if (!ensure(PlayerSetting != nullptr))
	{
		return;
	}

	const float MaxVelocity = PlayerSetting->MaxVelocity;
	const float Acceleration = PlayerSetting->Acceleration;
	MovementVelocity += Forward * Acceleration * DeltaTime;
	MovementVelocity = FMath::Clamp(MovementVelocity, -MaxVelocity, MaxVelocity);
}

void AFGPlayer::Server_SendYaw_Implementation(float NewYaw)
{
	ReplicatedYaw = NewYaw;
}

void AFGPlayer::ShowDebugMenu()
{
	CreateDebugWidget();

	if (DebugMenuInstance == nullptr)
	{
		return;
	}

	DebugMenuInstance->SetVisibility(ESlateVisibility::Visible);
	DebugMenuInstance->BP_OnShowWidget();
}

void AFGPlayer::HideDebugMenu()
{
	if (DebugMenuInstance == nullptr)
	{
		return;
	}

	DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	DebugMenuInstance->BP_OnHideWidget();
}

void AFGPlayer::TakeSomeDamage(float InDamage)
{
	if (HasAuthority())
	{
		Health -= InDamage;
		BP_OnHealthChanged(Health);
	}
}

void AFGPlayer::OnRep_HealthChanged()
{
	BP_OnHealthChanged(Health);
}

void AFGPlayer::OnPickup(APickup* Pickup)
{
	//if (IsLocallyControlled())
	//{
	//	Server_OnPickup(Pickup);
	//}

	if (HasAuthority())
	{
		ServerNumRockets += Pickup->NumRockets;
		NumRockets += Pickup->NumRockets;
		BP_OnNumRocketsChanged(NumRockets);
	}
}

void AFGPlayer::FireRocket()
{
	//UE_LOG(LogTemp, Warning, TEXT("NumRockets: %i, ServerNumRockets: %i"), NumRockets, ServerNumRockets);

	if (FireCooldownElapsed > 0.0f)
	{
		return;
	}

	if (NumRockets <= 0 && !bUnlimitedRockets)
	{
		return;
	}

	if (GetNumActiveRockets() >= MaxActiveRockets)
	{
		return;
	}

	UFGRocketComponent* NewRocket = GetFreeRocket();

	if (!ensure(NewRocket != nullptr))
	{
		return;
	}

	FireCooldownElapsed = PlayerSetting->FireCooldown;

	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		if (HasAuthority())
		{
			Server_FireRocket(NewRocket, GetRocketStartLocation(), GetActorRotation());
		}
		else
		{
			NumRockets--;
			BP_OnNumRocketsChanged(NumRockets);
			NewRocket->StartMoving(GetActorForwardVector(), GetRocketStartLocation());
			Server_FireRocket(NewRocket, GetRocketStartLocation(), GetActorRotation());
		}
	}
}

void AFGPlayer::OnRep_NumRocketsChanged()
{
	BP_OnNumRocketsChanged(NumRockets);
	UE_LOG(LogTemp, Warning, TEXT("OnRep_NumRocketsChanged NumRockets: %i"), NumRockets);
}

void AFGPlayer::Server_FireRocket_Implementation(UFGRocketComponent* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation)
{
	if ((ServerNumRockets - 1) < 0 && !bUnlimitedRockets)
	{
		Client_RemoveRocket(NewRocket);
	}
	else
	{
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(RocketFacingRotation.Yaw, GetActorForwardVector().Rotation().Yaw) * 0.5f;
		const FRotator NewFacingRotation = RocketFacingRotation + FRotator(0.0f, DeltaYaw, 0.0f);
		ServerNumRockets--;
		BP_OnNumRocketsChanged(ServerNumRockets);
		Multicast_FireRocket(NewRocket, RocketStartLocation, NewFacingRotation);
	}
}

void AFGPlayer::Multicast_FireRocket_Implementation(UFGRocketComponent* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation)
{
	if (!ensure(NewRocket != nullptr))
	{
		return;
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		NewRocket->ApplyCorrection(RocketFacingRotation.Vector());
	}
	else
	{
		NumRockets--;
		BP_OnNumRocketsChanged(NumRockets);
		NewRocket->StartMoving(RocketFacingRotation.Vector(), RocketStartLocation);
	}
}

void AFGPlayer::Client_RemoveRocket_Implementation(UFGRocketComponent* RocketToRemove)
{
	RocketToRemove->MakeFree();
}

void AFGPlayer::Cheat_IncreaseRockets(int32 InNumRockets)
{
	if (IsLocallyControlled())
	{
		NumRockets += InNumRockets;
	}
}

//void AFGPlayer::SpawnRockets()
//{
//	if (HasAuthority() && RocketClass != nullptr)
//	{
//		const int32 RocketCache = 8;
//
//		for (int32 Index = 0; Index < RocketCache; ++Index)
//		{
//			FActorSpawnParameters SpawnParams;
//			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
//			SpawnParams.ObjectFlags = RF_Transient;
//			SpawnParams.Instigator = this;
//			SpawnParams.Owner = this;
//			UFGRocketComponent* NewRocketInstance = GetWorld()->SpawnActor<UFGRocketComponent>(RocketClass, GetActorLocation(), GetActorRotation(), SpawnParams);
//			RocketInstances.Add(NewRocketInstance);
//
//			UFGRocketComponent* NewRocketInstance = NewObject<UFGRocketComponent>(this, RocketClass);			
//			RocketInstances.Add(NewRocketInstance);
//		}
//	}
//}

FVector AFGPlayer::GetRocketStartLocation() const
{
	const FVector StartLocation = GetActorLocation() + GetActorForwardVector() * 100.f;
	return StartLocation;
}

UFGRocketComponent* AFGPlayer::GetFreeRocket() const
{
	for (UFGRocketComponent* Rocket : RocketInstances)
	{
		if (Rocket == nullptr)
		{
			continue;
		}

		if (Rocket->IsFree())
		{
			return Rocket;
		}
	}

	return nullptr;
}

int32 AFGPlayer::GetNumActiveRockets() const
{
	int32 NumActive = 0;
	for (UFGRocketComponent* Rocket : RocketInstances)
	{
		if (!Rocket->IsFree())
		{
			NumActive++;
		}
	}
	return NumActive;
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

void AFGPlayer::Handle_DebugMenuPressed()
{
	bShowDebugMenu = !bShowDebugMenu;

	if (bShowDebugMenu)
	{
		ShowDebugMenu();
	}
	else
	{
		HideDebugMenu();
	}
}

void AFGPlayer::Handle_FirePressed()
{
	FireRocket();
}

void AFGPlayer::CreateDebugWidget()
{
	if (DebugMenuClass == nullptr)
	{
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	if (DebugMenuInstance == nullptr)
	{
		DebugMenuInstance = CreateWidget<UDebugWidget>(GetWorld(), DebugMenuClass);
		DebugMenuInstance->AddToViewport();
	}
}

void AFGPlayer::Server_OnPickup_Implementation(APickup* Pickup)
{
	//ServerNumRockets += Pickup->NumRockets;

	//NumRockets += Pickup->NumRockets;
	//	
	//Client_OnPickupRockets(NumRockets);

	//BP_OnNumRocketsChanged(NumRockets);

	//Client_OnPickupRockets(Pickup->NumRockets);
}

void AFGPlayer::Client_OnPickupRockets_Implementation(int32 PickedUpRockets)
{
	//NumRockets += PickedUpRockets;
}

void AFGPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFGPlayer, ReplicatedYaw);
	DOREPLIFETIME(AFGPlayer, ReplicatedLocation);
	DOREPLIFETIME(AFGPlayer, RocketInstances);
	DOREPLIFETIME(AFGPlayer, NumRockets);
	DOREPLIFETIME(AFGPlayer, Health);
}