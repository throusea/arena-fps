// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetGameMode.h"
#include "Variant_Network/NetGameState.h"
#include "Variant_Network/NetNPC.h"
#include "Variant_Network/NetPlayerStateBase.h"

ANetGameMode::ANetGameMode()
{
	// stub
}

void ANetGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ANetGameState* NetGameState = GetGameState<ANetGameState>())
	{
		NetGameState->SetTargetScore(TargetScore);
	}
}

void ANetGameMode::NotifyEnemyKilled(ANetNPC* KilledEnemy, AController* KillerController)
{
	if (!KilledEnemy || !KillerController)
	{
		return;
	}

	ANetPlayerStateBase* ScoringPlayerState = KillerController->GetPlayerState<ANetPlayerStateBase>();
	if (!ScoringPlayerState)
	{
		return;
	}

	if (ANetGameState* NetGameState = GetGameState<ANetGameState>())
	{
		if (NetGameState->HasWinner())
		{
			return;
		}
	}

	ScoringPlayerState->AddKillScore(ScorePerEnemyKill);
	CheckVictoryCondition(ScoringPlayerState);
}

void ANetGameMode::CheckVictoryCondition(ANetPlayerStateBase* ScoringPlayerState)
{
	if (!ScoringPlayerState || ScoringPlayerState->GetKillScore() < TargetScore)
	{
		return;
	}

	if (ANetGameState* NetGameState = GetGameState<ANetGameState>())
	{
		NetGameState->SetWinner(ScoringPlayerState);
	}
}
