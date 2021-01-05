// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerSetting.generated.h"

/**
 * 
 */
UCLASS()
class FGNET_API UPlayerSetting : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Movement)
	float Acceleration = 500.f;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (DisplayName = "TurnSpeed"))
	float TurnSpeedDefault = 100.f;

	UPROPERTY(EditAnywhere, Category = Movement)
	float MaxVelocity = 2000.f;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float Friction = 0.75f;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (ClampMin = 0.f, ClampMax = 1.f))
	float BrakingFriction = 0.001f;

	UPROPERTY(EditAnywhere, Category = Fire, meta = (ClampMin = 0.f))
	float FireCooldown = 0.15f;
};
