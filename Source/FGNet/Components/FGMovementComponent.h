#pragma once

#include "GameFramework/MovementComponent.h"
#include "FGMovementComponent.generated.h"


struct FFGFrameMovement;

UCLASS()
class FGNET_API UFGMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FFGFrameMovement CreateFrameMovement() const;

	void Move(FFGFrameMovement& FrameMovement);
	void ApplyGravity();

	UPROPERTY(EditAnywhere, Category = Movement)
	float Gravity = 30.0F;

	FVector GetGravityAsVector() const { return  { 0.0F, 0.0F, AccumulatedGravity }; }
	FRotator GetFacingRotation() const { return  FacingRotationCurrent; }
	FVector GetFacingDirection() const { return FacingRotationCurrent.Vector(); }

	void SetFacingRotation(const FRotator& InFacingRotation, float InRotationSpeed = -1.0F);
	void SetFacingRotation(const FQuat& InFacingRotation, float InRotationSpeed = -1.0F);
	void SetFacingDirection(const FVector& InFacingDirection, float InRotationSpeed = -1.0F);

private:
	void Internal_SetFacingRotation(const FRotator& InFacingRotation, float InRotationSpeed);
	FVector GetMovementDelta(const FFGFrameMovement& FrameMovement) const;

	FHitResult Hit;
	FRotator FacingRotationCurrent;
	FRotator FacingRotationTarget;
	float AccumulatedGravity = 0.0F;
	float FacingRotationSpeed = 1.0F;

};
