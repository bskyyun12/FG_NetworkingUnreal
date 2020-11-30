#pragma once

#include "GameFramework/Pawn.h"
#include "FGPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFGMovementComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class FGNET_API AFGPlayer : public APawn
{
	GENERATED_BODY()

public:
	AFGPlayer();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = Movement)
	float Acceleration = 500.0F;

	UPROPERTY(EditAnywhere, Category = Movement)
	float TurnSpeedDefault = 100.0F;

	UPROPERTY(EditAnywhere, Category = Movement)
	float MaxVelocity = 2000.0F;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float DefaultFriction = 0.75F;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float BrakingFriction = 0.001F;

	UFUNCTION(BlueprintPure)
	bool IsBraking() const { return bIsBraking; }

	UFUNCTION(BlueprintPure)
	int32 GetPing() const;

	UFUNCTION(Server, Unreliable)
	void Server_SendLocation(const FVector& LocationToSend);

	UFUNCTION(NetMulticast, Unreliable)
	void Mulitcast_SendLcation(const FVector& LocationToSend);

	UFUNCTION(Server, Unreliable)
	void Server_SendRotation(const FRotator& RotationToSend);

	UFUNCTION(NetMulticast, Unreliable)
	void Mulitcast_SendRotation(const FRotator& RotationToSend);

private:
	void Handle_Accelerate(float Value);
	void Handle_Turn(float Value);
	void Handle_BrakePressed();
	void Handle_BrakeReleased();

	float Forward = 0.0F;
	float Turn = 0.0F;

	float MovementVelocity = 0.0F;
	float Yaw = 0.0F;

	bool bIsBraking = false;

	UPROPERTY(VisibleDefaultsOnly, Category = Collision)
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleDefaultsOnly, Category =  Mesh)
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Camera)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Camera)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Movement)
	UFGMovementComponent* MovementComponent;
};
