#include "FGMovementStatics.h"

FFGFrameMovement::FFGFrameMovement(AActor* InActor)
{
	StartLocation = InActor->GetActorLocation();
}

FFGFrameMovement::FFGFrameMovement(USceneComponent* InSceneComponent)
{
	StartLocation = InSceneComponent->GetComponentLocation();
}

void FFGFrameMovement::AddDelta(const FVector& InDelta)
{
	MovementDelta += InDelta;
}
