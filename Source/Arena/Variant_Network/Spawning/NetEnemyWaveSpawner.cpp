// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Network/Spawning/NetEnemyWaveSpawner.h"
#include "Variant_Network/NetNPC.h"
#include "Variant_Network/Spawning/NetEnemySpawnPoint.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANetEnemyWaveSpawner::ANetEnemyWaveSpawner()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
}

void ANetEnemyWaveSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bStartOnBeginPlay)
	{
		StartWaves();
	}
}

void ANetEnemyWaveSpawner::StartWaves()
{
	if (!HasAuthority())
	{
		return;
	}

	StopWaves();

	bWavesRunning = true;
	CurrentWaveIndex = INDEX_NONE;
	StartNextWave();
}

void ANetEnemyWaveSpawner::StopWaves()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(WaveStartTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveCompletionTimerHandle);

	for (FNetEnemyWave& Wave : Waves)
	{
		for (ANetEnemySpawnPoint* SpawnPoint : Wave.SpawnPoints)
		{
			if (SpawnPoint)
			{
				SpawnPoint->OnEnemySpawned.RemoveDynamic(this, &ANetEnemyWaveSpawner::HandleSpawnPointEnemySpawned);
				SpawnPoint->StopSpawning();
			}
		}
	}

	ActiveEnemies.Reset();
	bWavesRunning = false;
	bCurrentWaveStarted = false;
}

void ANetEnemyWaveSpawner::StartNextWave()
{
	if (!HasAuthority() || !bWavesRunning)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(WaveStartTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveCompletionTimerHandle);

	++CurrentWaveIndex;
	bCurrentWaveStarted = false;
	ActiveEnemies.Reset();

	if (!Waves.IsValidIndex(CurrentWaveIndex))
	{
		bWavesRunning = false;
		CurrentWaveIndex = INDEX_NONE;
		OnAllWavesCompleted.Broadcast();
		return;
	}

	const FNetEnemyWave& CurrentWave = Waves[CurrentWaveIndex];
	if (CurrentWave.StartDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(WaveStartTimerHandle, this, &ANetEnemyWaveSpawner::StartCurrentWave, CurrentWave.StartDelay, false);
	}
	else
	{
		StartCurrentWave();
	}
}

int32 ANetEnemyWaveSpawner::GetAliveEnemyCount() const
{
	int32 AliveEnemyCount = 0;
	for (const TWeakObjectPtr<ANetNPC>& EnemyPtr : ActiveEnemies)
	{
		const ANetNPC* Enemy = EnemyPtr.Get();
		if (Enemy && !Enemy->IsDead())
		{
			++AliveEnemyCount;
		}
	}

	return AliveEnemyCount;
}

void ANetEnemyWaveSpawner::HandleSpawnPointEnemySpawned(ANetNPC* SpawnedEnemy)
{
	if (SpawnedEnemy)
	{
		ActiveEnemies.AddUnique(SpawnedEnemy);
	}
}

void ANetEnemyWaveSpawner::StartCurrentWave()
{
	if (!HasAuthority() || !bWavesRunning || !Waves.IsValidIndex(CurrentWaveIndex))
	{
		return;
	}

	const FNetEnemyWave& CurrentWave = Waves[CurrentWaveIndex];
	for (ANetEnemySpawnPoint* SpawnPoint : CurrentWave.SpawnPoints)
	{
		if (!SpawnPoint)
		{
			continue;
		}

		SpawnPoint->OnEnemySpawned.AddUniqueDynamic(this, &ANetEnemyWaveSpawner::HandleSpawnPointEnemySpawned);
		const TArray<ANetNPC*> SpawnedEnemies = SpawnPoint->SpawnEnemies(CurrentWave.SpawnCountPerPoint);
		for (ANetNPC* SpawnedEnemy : SpawnedEnemies)
		{
			HandleSpawnPointEnemySpawned(SpawnedEnemy);
		}
	}

	bCurrentWaveStarted = true;
	OnWaveStarted.Broadcast(CurrentWaveIndex);
	GetWorldTimerManager().SetTimer(WaveCompletionTimerHandle, this, &ANetEnemyWaveSpawner::CheckWaveCompletion, CompletionCheckInterval, true);
}

void ANetEnemyWaveSpawner::CheckWaveCompletion()
{
	if (!HasAuthority() || !bWavesRunning || !bCurrentWaveStarted)
	{
		return;
	}

	CleanupActiveEnemies();

	if (Waves.IsValidIndex(CurrentWaveIndex))
	{
		const FNetEnemyWave& CurrentWave = Waves[CurrentWaveIndex];
		for (const ANetEnemySpawnPoint* SpawnPoint : CurrentWave.SpawnPoints)
		{
			if (SpawnPoint && SpawnPoint->IsSpawning())
			{
				return;
			}
		}
	}

	if (ActiveEnemies.Num() > 0)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(WaveCompletionTimerHandle);
	OnWaveCompleted.Broadcast(CurrentWaveIndex);
	StartNextWave();
}

void ANetEnemyWaveSpawner::CleanupActiveEnemies()
{
	ActiveEnemies.RemoveAll(
		[](const TWeakObjectPtr<ANetNPC>& EnemyPtr)
		{
			const ANetNPC* Enemy = EnemyPtr.Get();
			return !Enemy || Enemy->IsDead();
		}
	);
}
