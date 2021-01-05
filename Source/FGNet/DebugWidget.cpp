// Fill out your copyright notice in the Description page of Project Settings.

#include "DebugWidget.h"
#include "Engine/World.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/DefaultValueHelper.h"

void UDebugWidget::UpdateNetworkSimulationSettings(const FFGBlueprintNetworkSimulationSettings& InPackets)
{
	if (UWorld* World = GetWorld())
	{
		if (World->GetNetDriver() != nullptr)
		{
			FPacketSimulationSettings PacketSimulation;
			PacketSimulation.PktLagMin = InPackets.MinLatency;
			PacketSimulation.PktLagMax = InPackets.MaxLatency;
			PacketSimulation.PktLoss = InPackets.PacketLossPercentage;
			PacketSimulation.PktIncomingLagMin = InPackets.MinLatency;
			PacketSimulation.PktIncomingLagMax = InPackets.MaxLatency;
			PacketSimulation.PktIncomingLoss = InPackets.PacketLossPercentage;
			World->GetNetDriver()->SetPacketSimulationSettings(PacketSimulation);

			FFGBlueprintNetworkSimulationSettingsText SimulationSettingsText;
			SimulationSettingsText.MaxLatency = FText::FromString(FString::FromInt(InPackets.MaxLatency));
			SimulationSettingsText.MinLatency = FText::FromString(FString::FromInt(InPackets.MinLatency));
			SimulationSettingsText.PacketLossPercentage = FText::FromString(FString::FromInt(InPackets.PacketLossPercentage));

			BP_OnUpdateNetworkSimulationSettings(SimulationSettingsText);
		}
	}
}

void UDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APlayerState* PlayerState = PC->GetPlayerState<APlayerState>())
		{
			BP_UpdatePing(static_cast<int32>(PlayerState->GetPing()));
		}
	}
}
