// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NetGameMode.generated.h"

class ANetNPC;
class ANetCharacter;
class ANetPlayerStateBase;
class APawn;

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
	virtual void Logout(AController* Exiting) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	/** Awards score for a defeated enemy and updates victory state. */
	void NotifyEnemyKilled(ANetNPC* KilledEnemy, AController* KillerController);

	/** Awards score when one player defeats another player. */
	void NotifyPlayerKilled(ANetCharacter* KilledPlayer, AController* KillerController);

	/** Schedules a server-authoritative restart for a dead player pawn. */
	void RequestPlayerRespawn(AController* Controller, APawn* DeadPawn);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scoring", meta=(ClampMin=1))
	int32 ScorePerEnemyKill = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scoring", meta=(ClampMin=1))
	int32 ScorePerPlayerKill = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scoring", meta=(ClampMin=1))
	int32 TargetScore = 10;

	/** Time the player remains on the death camera before a new pawn is spawned. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Respawn", meta=(ClampMin=0, Units="s"))
	float RespawnDelay = 5.0f;

private:
	void CheckVictoryCondition(ANetPlayerStateBase* ScoringPlayerState);
	void AwardScore(AController* ScoringController, int32 ScoreAmount);
	void RespawnPlayer(TWeakObjectPtr<AController> Controller, TWeakObjectPtr<APawn> DeadPawn);
	void CancelPendingRespawn(AController* Controller);
	bool CanRespawnPlayers() const;

	TMap<TWeakObjectPtr<AController>, FTimerHandle> PendingRespawns;
};
