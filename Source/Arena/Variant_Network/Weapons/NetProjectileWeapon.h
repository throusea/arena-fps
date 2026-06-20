// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NetRifle.h"
#include "NetProjectileWeapon.generated.h"

class ANetProjectile;

/** Network weapon that spawns a replicated server-authoritative projectile. */
UCLASS(Blueprintable)
class ARENA_API ANetProjectileWeapon : public ANetWeaponBase
{
	GENERATED_BODY()

public:
	virtual ENetWeaponFireType GetFireType() const override { return ENetWeaponFireType::Projectile; }

protected:
	virtual bool CanFireAuthoritativeShot() const override;
	virtual bool FireAuthoritativeShot(FNetWeaponFireResult& OutFireResult, FNetWeaponImpactResult& OutImpactResult) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Projectile")
	TSubclassOf<ANetProjectile> ProjectileClass;

private:
	friend class ANetProjectile;
	void HandleProjectileImpact(const FHitResult& HitResult, bool bDamagedActor);
};
