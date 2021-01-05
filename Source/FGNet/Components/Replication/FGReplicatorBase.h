// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FGReplicatorBase.generated.h"

UENUM()
enum class EFGSmoothRelicatorMode : uint8
{
	ConstantVelocity
};

template<typename ValueType>
struct TFGSmoothReplicatorOperation
{
	static void InterpConstantVelocity(ValueType& CurrentValue, const ValueType& FrameTarget, float Alpha)
	{
		CurrentValue = CurrentValue + (FrameTarget - CurrentValue) * Alpha;
	}
};

UCLASS(abstract, BlueprintType, Blueprintable)
class FGNET_API UFGReplicatorBase : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Init() {}

	// UObject
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;
	virtual bool IsSupportedForNetworking() const override;
	virtual bool IsNameStableForNetworking() const override;
	// UObject

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject

	void SetShouldTick(bool bInShouldTick);
	bool IsTicking() const;

	bool IsLocallyControlled() const;
	bool HasAuthority() const;

private:
	bool bShouldTick = false;
};
