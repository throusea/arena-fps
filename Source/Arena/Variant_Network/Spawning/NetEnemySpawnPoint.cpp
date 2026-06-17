// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Network/Spawning/NetEnemySpawnPoint.h"
#include "Variant_Network/NetNPC.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANetEnemySpawnPoint::ANetEnemySpawnPoint()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
}

void ANetEnemySpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bSpawnOnBeginPlay)
	{
		SpawnEnemies();
	}
}

TArray<ANetNPC*> ANetEnemySpawnPoint::SpawnEnemies(int32 CountOverride)
{
	TArray<ANetNPC*> ImmediatelySpawnedEnemies;

	if (!HasAuthority())
	{
		return ImmediatelySpawnedEnemies;
	}

	StopSpawning();
	CleanupSpawnedEnemies();

	RemainingQueuedSpawns = CountOverride > 0 ? CountOverride : SpawnCount;
	if (RemainingQueuedSpawns <= 0)
	{
		return ImmediatelySpawnedEnemies;
	}

	if (SpawnInterval <= 0.0f)
	{
		while (RemainingQueuedSpawns > 0)
		{
			if (ANetNPC* SpawnedEnemy = SpawnSingleEnemy())
			{
				ImmediatelySpawnedEnemies.Add(SpawnedEnemy);
			}
			--RemainingQueuedSpawns;
		}

		return ImmediatelySpawnedEnemies;
	}

	if (ANetNPC* SpawnedEnemy = SpawnSingleEnemy())
	{
		ImmediatelySpawnedEnemies.Add(SpawnedEnemy);
	}
	--RemainingQueuedSpawns;

	if (RemainingQueuedSpawns > 0)
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ANetEnemySpawnPoint::SpawnNextQueuedEnemy, SpawnInterval, true);
	}

	return ImmediatelySpawnedEnemies;
}

void ANetEnemySpawnPoint::StopSpawning()
{
	RemainingQueuedSpawns = 0;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
}

TArray<ANetNPC*> ANetEnemySpawnPoint::GetSpawnedEnemies() const
{
	TArray<ANetNPC*> ValidEnemies;
	for (const TWeakObjectPtr<ANetNPC>& EnemyPtr : SpawnedEnemies)
	{
		if (ANetNPC* Enemy = EnemyPtr.Get())
		{
			ValidEnemies.Add(Enemy);
		}
	}

	return ValidEnemies;
}

ANetNPC* ANetEnemySpawnPoint::SpawnSingleEnemy()
{
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s cannot spawn because EnemyClass is not set."), *GetName());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = SpawnCollisionHandling;

	ANetNPC* SpawnedEnemy = World->SpawnActor<ANetNPC>(EnemyClass, GetActorTransform(), SpawnParams);
	if (!SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s failed to spawn enemy class %s."), *GetName(), *GetNameSafe(EnemyClass.Get()));
		return nullptr;
	}

	SpawnedEnemies.Add(SpawnedEnemy);
	OnEnemySpawned.Broadcast(SpawnedEnemy);

	return SpawnedEnemy;
}

void ANetEnemySpawnPoint::SpawnNextQueuedEnemy()
{
	if (!HasAuthority() || RemainingQueuedSpawns <= 0)
	{
		StopSpawning();
		return;
	}

	SpawnSingleEnemy();
	--RemainingQueuedSpawns;

	if (RemainingQueuedSpawns <= 0)
	{
		StopSpawning();
	}
}

void ANetEnemySpawnPoint::CleanupSpawnedEnemies()
{
	SpawnedEnemies.RemoveAll(
		[](const TWeakObjectPtr<ANetNPC>& EnemyPtr)
		{
			return !EnemyPtr.IsValid();
		}
	);
}
