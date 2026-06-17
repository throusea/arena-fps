// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NetGameMode.generated.h"

class ANetNPC;
class ANetPlayerStateBase;

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class ANetGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANetGameMode();

	virtual void BeginPlay() override;

	/** Awards score for a defeated enemy and updates victory state. */
	void NotifyEnemyKilled(ANetNPC* KilledEnemy, AController* KillerController);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scoring", meta=(ClampMin=1))
	int32 ScorePerEnemyKill = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scoring", meta=(ClampMin=1))
	int32 TargetScore = 10;

private:
	void CheckVictoryCondition(ANetPlayerStateBase* ScoringPlayerState);
};
