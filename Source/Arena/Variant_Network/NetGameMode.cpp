// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetGameMode.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Variant_Network/NetGameState.h"
#include "Variant_Network/NetCharacter.h"
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

void ANetGameMode::Logout(AController* Exiting)
{
	CancelPendingRespawn(Exiting);
	Super::Logout(Exiting);
}

bool ANetGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// Returning false makes every restart use the engine's random, occupancy-aware PlayerStart selection.
	return false;
}

void ANetGameMode::RequestPlayerRespawn(AController* Controller, APawn* DeadPawn)
{
	if (!HasAuthority() || !IsValid(Controller) || !IsValid(DeadPawn)
		|| Controller->GetPawn() != DeadPawn || PendingRespawns.Contains(Controller)
		|| !CanRespawnPlayers())
	{
		return;
	}

	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(
		this,
		&ANetGameMode::RespawnPlayer,
		TWeakObjectPtr<AController>(Controller),
		TWeakObjectPtr<APawn>(DeadPawn));

	FTimerHandle& RespawnTimer = PendingRespawns.Add(Controller);
	if (RespawnDelay <= 0.0f)
	{
		RespawnTimer = GetWorldTimerManager().SetTimerForNextTick(RespawnDelegate);
	}
	else
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, RespawnDelay, false);
	}
}

void ANetGameMode::NotifyEnemyKilled(ANetNPC* KilledEnemy, AController* KillerController)
{
	if (!KilledEnemy || !KillerController)
	{
		return;
	}

	AwardScore(KillerController, ScorePerEnemyKill);
}

void ANetGameMode::NotifyPlayerKilled(ANetCharacter* KilledPlayer, AController* KillerController)
{
	if (!KilledPlayer || !KillerController || KilledPlayer->GetController() == KillerController)
	{
		return;
	}

	AwardScore(KillerController, ScorePerPlayerKill);
}

void ANetGameMode::AwardScore(AController* ScoringController, int32 ScoreAmount)
{
	if (!ScoringController || ScoreAmount <= 0)
	{
		return;
	}

	ANetPlayerStateBase* ScoringPlayerState = ScoringController->GetPlayerState<ANetPlayerStateBase>();
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

	ScoringPlayerState->AddKillScore(ScoreAmount);
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

void ANetGameMode::RespawnPlayer(TWeakObjectPtr<AController> Controller, TWeakObjectPtr<APawn> DeadPawn)
{
	PendingRespawns.Remove(Controller);

	AController* RespawningController = Controller.Get();
	if (!IsValid(RespawningController))
	{
		return;
	}

	if (!CanRespawnPlayers())
	{
		return;
	}

	APawn* CurrentPawn = RespawningController->GetPawn();
	if (CurrentPawn && CurrentPawn != DeadPawn.Get())
	{
		return;
	}

	if (IsValid(CurrentPawn))
	{
		RespawningController->UnPossess();
		CurrentPawn->Destroy();
	}

	if (RespawningController->GetPawn() != nullptr)
	{
		return;
	}

	RestartPlayer(RespawningController);
}

void ANetGameMode::CancelPendingRespawn(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	if (FTimerHandle* RespawnTimer = PendingRespawns.Find(Controller))
	{
		GetWorldTimerManager().ClearTimer(*RespawnTimer);
		PendingRespawns.Remove(Controller);
	}
}

bool ANetGameMode::CanRespawnPlayers() const
{
	const ANetGameState* NetGameState = GetGameState<ANetGameState>();
	return !NetGameState || !NetGameState->HasWinner();
}
