// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetEnemySpawnPoint.generated.h"

class ANetNPC;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetEnemySpawnedSignature, ANetNPC*, SpawnedEnemy);

/**
 * Server-authoritative spawn point for network PvE enemies.
 */
UCLASS(Blueprintable)
class ARENA_API ANetEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ANetEnemySpawnPoint();

	/** Starts spawning enemies from this point. Returns enemies spawned immediately. */
	UFUNCTION(BlueprintCallable, Category="Spawning")
	TArray<ANetNPC*> SpawnEnemies(int32 CountOverride = -1);

	/** Stops any pending interval-based spawning. */
	UFUNCTION(BlueprintCallable, Category="Spawning")
	void StopSpawning();

	/** Returns spawned enemies that are still valid. */
	UFUNCTION(BlueprintPure, Category="Spawning")
	TArray<ANetNPC*> GetSpawnedEnemies() const;

	UFUNCTION(BlueprintPure, Category="Spawning")
	bool IsSpawning() const { return RemainingQueuedSpawns > 0; }

	UPROPERTY(BlueprintAssignable, Category="Spawning")
	FNetEnemySpawnedSignature OnEnemySpawned;

protected:
	virtual void BeginPlay() override;

private:
	ANetNPC* SpawnSingleEnemy();
	void SpawnNextQueuedEnemy();
	void CleanupSpawnedEnemies();

private:
	/** Enemy blueprint or C++ class to spawn at this point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta=(AllowPrivateAccess="true"))
	TSubclassOf<ANetNPC> EnemyClass;

	/** Default number of enemies to spawn when no override is provided. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta=(AllowPrivateAccess="true", ClampMin=1))
	int32 SpawnCount = 1;

	/** Delay between spawned enemies when spawning a batch. Zero spawns the whole batch immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta=(AllowPrivateAccess="true", ClampMin=0, Units="s"))
	float SpawnInterval = 0.0f;

	/** Starts a default spawn batch on BeginPlay. Only runs on the server. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta=(AllowPrivateAccess="true"))
	bool bSpawnOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta=(AllowPrivateAccess="true"))
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ANetNPC>> SpawnedEnemies;

	FTimerHandle SpawnTimerHandle;
	int32 RemainingQueuedSpawns = 0;
};
