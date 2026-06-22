// Fill out your copyright notice in the Description page of Project Settings.

#include "NetGameState.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Network/NetPlayerStateBase.h"

void ANetGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetGameState, TargetScore);
	DOREPLIFETIME(ANetGameState, bHasWinner);
	DOREPLIFETIME(ANetGameState, WinnerPlayerState);
}

void ANetGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	BindToPlayerState(Cast<ANetPlayerStateBase>(PlayerState));
	OnScoreboardChanged.Broadcast();
}

void ANetGameState::RemovePlayerState(APlayerState* PlayerState)
{
	UnbindFromPlayerState(Cast<ANetPlayerStateBase>(PlayerState));
	Super::RemovePlayerState(PlayerState);
	OnScoreboardChanged.Broadcast();
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

TArray<FNetScoreboardEntry> ANetGameState::GetScoreboardEntries() const
{
	TArray<FNetScoreboardEntry> Entries;
	Entries.Reserve(PlayerArray.Num());

	for (APlayerState* PlayerState : PlayerArray)
	{
		if (ANetPlayerStateBase* NetPlayerState = Cast<ANetPlayerStateBase>(PlayerState))
		{
			FNetScoreboardEntry& Entry = Entries.AddDefaulted_GetRef();
			Entry.PlayerState = NetPlayerState;
			Entry.PlayerName = NetPlayerState->GetPlayerName();
			Entry.KillScore = NetPlayerState->GetKillScore();
		}
	}

	Entries.Sort([](const FNetScoreboardEntry& Left, const FNetScoreboardEntry& Right)
	{
		if (Left.KillScore != Right.KillScore)
		{
			return Left.KillScore > Right.KillScore;
		}

		return Left.PlayerName.Compare(Right.PlayerName, ESearchCase::IgnoreCase) < 0;
	});

	return Entries;
}

void ANetGameState::HandlePlayerScoreChanged(int32 NewScore)
{
	OnScoreboardChanged.Broadcast();
}

void ANetGameState::HandlePlayerNameChanged(const FString& NewPlayerName)
{
	OnScoreboardChanged.Broadcast();
}

void ANetGameState::BindToPlayerState(ANetPlayerStateBase* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	PlayerState->OnKillScoreChanged.AddUniqueDynamic(this, &ANetGameState::HandlePlayerScoreChanged);
	PlayerState->OnPlayerNameChanged.AddUniqueDynamic(this, &ANetGameState::HandlePlayerNameChanged);
}

void ANetGameState::UnbindFromPlayerState(ANetPlayerStateBase* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	PlayerState->OnKillScoreChanged.RemoveDynamic(this, &ANetGameState::HandlePlayerScoreChanged);
	PlayerState->OnPlayerNameChanged.RemoveDynamic(this, &ANetGameState::HandlePlayerNameChanged);
}
