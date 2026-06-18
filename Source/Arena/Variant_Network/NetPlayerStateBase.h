// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetPlayerStateBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetPlayerScoreChangedSignature, int32, NewScore);

/**
 *
 */
UCLASS()
class ARENA_API ANetPlayerStateBase : public APlayerState
{
	GENERATED_BODY()

public:
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	// UAbilitySystemComponent* Asc;

public:

	ANetPlayerStateBase();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Adds score on the authoritative instance. */
	void AddKillScore(int32 ScoreDelta);

	UFUNCTION(BlueprintPure, Category="Score")
	int32 GetKillScore() const { return KillScore; }

	UPROPERTY(BlueprintAssignable, Category="Score")
	FNetPlayerScoreChangedSignature OnKillScoreChanged;

	// virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	UFUNCTION()
	void OnRep_KillScore();

private:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_KillScore, BlueprintReadOnly, Category="Score", meta=(AllowPrivateAccess="true"))
	int32 KillScore = 0;
};
