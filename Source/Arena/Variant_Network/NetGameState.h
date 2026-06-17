// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NetGameState.generated.h"

class ANetPlayerStateBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNetVictoryStateChangedSignature);

/**
 *
 */
UCLASS()
class ARENA_API ANetGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetTargetScore(int32 NewTargetScore);
	void SetWinner(ANetPlayerStateBase* NewWinner);

	UFUNCTION(BlueprintPure, Category="Victory")
	int32 GetTargetScore() const { return TargetScore; }

	UFUNCTION(BlueprintPure, Category="Victory")
	bool HasWinner() const { return bHasWinner; }

	UFUNCTION(BlueprintPure, Category="Victory")
	ANetPlayerStateBase* GetWinnerPlayerState() const { return WinnerPlayerState; }

	UPROPERTY(BlueprintAssignable, Category="Victory")
	FNetVictoryStateChangedSignature OnVictoryStateChanged;

private:
	UFUNCTION()
	void OnRep_TargetScore();

	UFUNCTION()
	void OnRep_VictoryState();

private:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_TargetScore, BlueprintReadOnly, Category="Victory", meta=(AllowPrivateAccess="true"))
	int32 TargetScore = 10;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_VictoryState, BlueprintReadOnly, Category="Victory", meta=(AllowPrivateAccess="true"))
	bool bHasWinner = false;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_VictoryState, BlueprintReadOnly, Category="Victory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ANetPlayerStateBase> WinnerPlayerState;
};
