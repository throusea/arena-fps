// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetProjectileWeapon.h"
#include "NetProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

bool ANetProjectileWeapon::CanFireAuthoritativeShot() const
{
	return Super::CanFireAuthoritativeShot() && ProjectileClass != nullptr;
}

bool ANetProjectileWeapon::FireAuthoritativeShot(
	FNetWeaponFireResult& OutFireResult,
	FNetWeaponImpactResult& OutImpactResult)
{
	LastHitResult = FHitResult();

	FVector TraceStart;
	FVector ShotDirection;
	FVector TraceEnd;
	if (!CalculateShotPath(TraceStart, ShotDirection, TraceEnd))
	{
		return false;
	}

	USkeletalMeshComponent* MuzzleMesh = GetThirdPersonMesh();
	const USceneComponent* AttachParent = MuzzleMesh ? MuzzleMesh->GetAttachParent() : nullptr;
	const bool bHasAttachedMuzzle = AttachParent
		&& AttachParent->GetOwner() == PawnOwner
		&& MuzzleMesh->DoesSocketExist(MuzzleSocketName);
	const FVector MuzzleLocation = bHasAttachedMuzzle
		? MuzzleMesh->GetSocketLocation(MuzzleSocketName)
		: TraceStart;
	const FVector ProjectileDirection = (TraceEnd - MuzzleLocation).GetSafeNormal();
	const FVector SpawnLocation = MuzzleLocation + (ProjectileDirection * MuzzleOffset);
	const FTransform SpawnTransform(ProjectileDirection.Rotation(), SpawnLocation);

	OutFireResult.Origin = SpawnLocation;
	OutFireResult.Direction = ProjectileDirection;
	OutFireResult.SpreadAngle = GetCurrentSpreadAngle();

	if (!ProjectileClass)
	{
		return false;
	}

	ANetProjectile* Projectile = GetWorld()->SpawnActorDeferred<ANetProjectile>(
		ProjectileClass,
		SpawnTransform,
		PawnOwner,
		PawnOwner,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		Projectile->InitializeProjectile(this);
		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
		return true;
	}

	return false;
}

void ANetProjectileWeapon::HandleProjectileImpact(const FHitResult& HitResult, bool bDamagedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	LastHitResult = HitResult;

	FNetWeaponImpactResult ImpactResult;
	ImpactResult.ImpactLocation = HitResult.ImpactPoint;
	ImpactResult.ImpactNormal = HitResult.ImpactNormal;
	ImpactResult.HitActor = HitResult.GetActor();
	ImpactResult.bBlockingHit = HitResult.bBlockingHit;
	ImpactResult.bDamagedActor = bDamagedActor;
	MulticastPlayHitEffects(ImpactResult);
}
