#pragma once

#include "GameFramework/Pawn.h"
#include "FGPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFGMovementComponent;
class UStaticMeshComponent;
class USphereComponent;
class UPlayerSetting;
class UDebugWidget;
class APickup;
class AFGRocket;

UCLASS()
class FGNET_API AFGPlayer : public APawn
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	AFGPlayer();


	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = Settings)
	UPlayerSetting* PlayerSetting = nullptr;

	UFUNCTION(BlueprintPure)
	bool IsBraking() const { return bIsBraking; }

	UFUNCTION(BlueprintPure)
	int32 GetPing() const;

	UPROPERTY(EditAnywhere, Category = Debug)
	TSubclassOf<UDebugWidget> DebugMenuClass;

	UFUNCTION(Server, Unreliable)
	void Server_SendTransform(const FTransform& TransformToSend);

	UFUNCTION(Server, Unreliable)
	void Server_SendYaw(float NewYaw);

	void OnPickup(APickup* Pickup);

	UFUNCTION(Server, Reliable)
	void Server_OnPickup(APickup* Pickup);

	UFUNCTION(Client, Reliable)
	void Client_OnPickupRockets(int32 PickedUpRockets);

	UFUNCTION(NetMulticast, Unreliable)
	void Mulitcast_SendTransform(const FTransform& TransformToSend);

	void ShowDebugMenu();
	void HideDebugMenu();

	UFUNCTION(BlueprintPure)
	int32 GetNumRockets() const { return NumRockets; }

	UFUNCTION(BlueprintImplementableEvent, Category = Player, meta = (DisplayName = "On Num Rockets Changed"))
	void BP_OnNumRocketsChanged(int32 NewNumRockets);

	//
	void FireRocket();
	void SpawnRockets();
	//

private:

	//
	int32 ServerNumRockets = 0;
	int32 NumRockets = 0;

	FVector GetRocketStartLocation() const;
	AFGRocket* GetFreeRocket() const;
	int32 GetNumActiveRockets() const;

	void AddMovementVelocity(float DeltaTime);

	UFUNCTION(Server, Unreliable)
	void Server_SendMovement(const FVector& ClientLocation, float TimeStamp, float ClientForward, float ClientYaw);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SendMovement(const FVector& InClientLocation, float TimeStamp, float ClientForward, float ClientYaw);

	UFUNCTION(Server, Reliable)
	void Server_FireRocket(AFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_FireRocket(AFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation);

	UFUNCTION(Client, Reliable)
	void Client_RemoveRocket(AFGRocket* RocketToRemove);

	UFUNCTION(BlueprintCallable)
	void Cheat_IncreaseRockets(int32 InNumRockets);

	UPROPERTY(Replicated, Transient)
	TArray<AFGRocket*> RocketInstances;

	UPROPERTY(EditAnywhere, Category = Weapon)
	TSubclassOf<AFGRocket> RocketClass;

	int32 MaxActiveRockets = 3;
	
	float FireCooldownElapsed = 0.0f;

	UPROPERTY(EditAnywhere, Category = Weapon)
	bool bUnlimitedRockets = false;
	//

	void Handle_Accelerate(float Value);
	void Handle_Turn(float Value);
	void Handle_BrakePressed();
	void Handle_BrakeReleased();
	void Handle_DebugMenuPressed();
	void Handle_FirePressed();

	void CreateDebugWidget();

	UPROPERTY(Transient)
	UDebugWidget* DebugMenuInstance = nullptr;

	UPROPERTY(EditAnywhere)
	bool bShowDebugMenu = false;

	UPROPERTY(Replicated)
	float ReplicatedYaw = 0.0f;

	UPROPERTY(Replicated)
	FVector ReplicatedLocation;

	float Forward = 0.0F;
	float Turn = 0.0F;

	float MovementVelocity = 0.0F;
	float Yaw = 0.0F;

	bool bIsBraking = false;

	//
	float ClientTimeStamp = 0.0f;
	float ServerTimeStamp = 0.0f;
	float LastCorrectionDelta = 0.0f;

	UPROPERTY(EditAnywhere, Category = Network)
	bool bPerformNetworkSmoothing = false;

	FVector OriginalMeshOffset;
	//

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

	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	FQuat TargetRotation;

	UPROPERTY()
	float LocationLerpSpeed = 5.f;

	UPROPERTY()
	float RotationLerpSpeed = 5.f;
};
