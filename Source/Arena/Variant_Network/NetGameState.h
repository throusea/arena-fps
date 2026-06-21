// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NetGameState.generated.h"

class ANetPlayerStateBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNetVictoryStateChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNetScoreboardChangedSignature);

USTRUCT(BlueprintType)
struct FNetScoreboardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
	TObjectPtr<ANetPlayerStateBase> PlayerState;

	UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category="Scoreboard")
	int32 KillScore = 0;
};

/**
 *
 */
UCLASS()
class ARENA_API ANetGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	void SetTargetScore(int32 NewTargetScore);
	void SetWinner(ANetPlayerStateBase* NewWinner);

	UFUNCTION(BlueprintPure, Category="Victory")
	int32 GetTargetScore() const { return TargetScore; }

	UFUNCTION(BlueprintPure, Category="Victory")
	bool HasWinner() const { return bHasWinner; }

	UFUNCTION(BlueprintPure, Category="Victory")
	ANetPlayerStateBase* GetWinnerPlayerState() const { return WinnerPlayerState; }

	/** Returns all network players sorted by score descending, then name. */
	UFUNCTION(BlueprintPure, Category="Scoreboard")
	TArray<FNetScoreboardEntry> GetScoreboardEntries() const;

	UPROPERTY(BlueprintAssignable, Category="Victory")
	FNetVictoryStateChangedSignature OnVictoryStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Scoreboard")
	FNetScoreboardChangedSignature OnScoreboardChanged;

private:
	UFUNCTION()
	void OnRep_TargetScore();

	UFUNCTION()
	void OnRep_VictoryState();

	UFUNCTION()
	void HandlePlayerScoreChanged(int32 NewScore);

	UFUNCTION()
	void HandlePlayerNameChanged(const FString& NewPlayerName);

	void BindToPlayerState(ANetPlayerStateBase* PlayerState);
	void UnbindFromPlayerState(ANetPlayerStateBase* PlayerState);

private:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_TargetScore, BlueprintReadOnly, Category="Victory", meta=(AllowPrivateAccess="true"))
	int32 TargetScore = 10;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_VictoryState, BlueprintReadOnly, Category="Victory", meta=(AllowPrivateAccess="true"))
	bool bHasWinner = false;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_VictoryState, BlueprintReadOnly, Category="Victory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ANetPlayerStateBase> WinnerPlayerState;
};
