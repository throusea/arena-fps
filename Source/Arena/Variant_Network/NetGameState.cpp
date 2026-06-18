// Fill out your copyright notice in the Description page of Project Settings.

#include "NetGameState.h"
#include "Net/UnrealNetwork.h"

void ANetGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetGameState, TargetScore);
	DOREPLIFETIME(ANetGameState, bHasWinner);
	DOREPLIFETIME(ANetGameState, WinnerPlayerState);
}

void ANetGameState::SetTargetScore(int32 NewTargetScore)
{
	if (!HasAuthority())
	{
		return;
	}

	TargetScore = FMath::Max(1, NewTargetScore);
	OnVictoryStateChanged.Broadcast();
}

void ANetGameState::SetWinner(ANetPlayerStateBase* NewWinner)
{
	if (!HasAuthority() || bHasWinner || !NewWinner)
	{
		return;
	}

	WinnerPlayerState = NewWinner;
	bHasWinner = true;
	OnVictoryStateChanged.Broadcast();
}

void ANetGameState::OnRep_TargetScore()
{
	OnVictoryStateChanged.Broadcast();
}

void ANetGameState::OnRep_VictoryState()
{
	OnVictoryStateChanged.Broadcast();
}
