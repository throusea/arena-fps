// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetEnemyWaveSpawner.generated.h"

class ANetEnemySpawnPoint;
class ANetNPC;

USTRUCT(BlueprintType)
struct FNetEnemyWave
{
	GENERATED_BODY()

	/** Spawn points used by this wave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wave")
	TArray<TObjectPtr<ANetEnemySpawnPoint>> SpawnPoints;

	/** Number of enemies each spawn point should create for this wave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wave", meta=(ClampMin=1))
	int32 SpawnCountPerPoint = 1;

	/** Delay before this wave begins after it is selected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wave", meta=(ClampMin=0, Units="s"))
	float StartDelay = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetEnemyWaveIndexSignature, int32, WaveIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNetEnemyWavesCompletedSignature);

/**
 * Minimal fixed-wave PvE spawner that drives a set of enemy spawn points on the server.
 */
UCLASS(Blueprintable)
class ARENA_API ANetEnemyWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANetEnemyWaveSpawner();

	UFUNCTION(BlueprintCallable, Category="Spawning|Waves")
	void StartWaves();

	UFUNCTION(BlueprintCallable, Category="Spawning|Waves")
	void StopWaves();

	UFUNCTION(BlueprintCallable, Category="Spawning|Waves")
	void StartNextWave();

	UFUNCTION(BlueprintPure, Category="Spawning|Waves")
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

	UFUNCTION(BlueprintPure, Category="Spawning|Waves")
	int32 GetAliveEnemyCount() const;

	UPROPERTY(BlueprintAssignable, Category="Spawning|Waves")
	FNetEnemyWaveIndexSignature OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category="Spawning|Waves")
	FNetEnemyWaveIndexSignature OnWaveCompleted;

	UPROPERTY(BlueprintAssignable, Category="Spawning|Waves")
	FNetEnemyWavesCompletedSignature OnAllWavesCompleted;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleSpawnPointEnemySpawned(ANetNPC* SpawnedEnemy);

	void StartCurrentWave();
	void CheckWaveCompletion();
	void CleanupActiveEnemies();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning|Waves", meta=(AllowPrivateAccess="true"))
	TArray<FNetEnemyWave> Waves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning|Waves", meta=(AllowPrivateAccess="true"))
	bool bStartOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning|Waves", meta=(AllowPrivateAccess="true", ClampMin=0.1, Units="s"))
	float CompletionCheckInterval = 0.5f;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ANetNPC>> ActiveEnemies;

	FTimerHandle WaveStartTimerHandle;
	FTimerHandle WaveCompletionTimerHandle;
	int32 CurrentWaveIndex = INDEX_NONE;
	bool bWavesRunning = false;
	bool bCurrentWaveStarted = false;
};
