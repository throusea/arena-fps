// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetProjectile.generated.h"

class ANetProjectileWeapon;
class UDamageType;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;

/** Server-authoritative replicated projectile with optional radial damage. */
UCLASS(Abstract, Blueprintable)
class ARENA_API ANetProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANetProjectile();

	void InitializeProjectile(ANetProjectileWeapon* InSourceWeapon);

	UFUNCTION(BlueprintPure, Category="Projectile")
	USphereComponent* GetCollisionComponent() const { return CollisionComponent; }

	UFUNCTION(BlueprintPure, Category="Projectile")
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

protected:
	virtual void BeginPlay() override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
		bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse,
		const FHitResult& Hit) override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleImpact(FVector_NetQuantize ImpactLocation, FVector_NetQuantizeNormal ImpactNormal, bool bExploded);

	UFUNCTION(BlueprintImplementableEvent, Category="Projectile", meta=(DisplayName="Projectile Impact"))
	void BP_OnProjectileImpact(FVector ImpactLocation, FVector ImpactNormal, bool bExploded);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Damage", meta=(ClampMin=0))
	float HitDamage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Damage")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Damage")
	bool bDamageInstigator = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Explosion")
	bool bExplodeOnImpact = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Explosion", meta=(EditCondition="bExplodeOnImpact", ClampMin=0))
	float ExplosionDamage = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Explosion", meta=(EditCondition="bExplodeOnImpact", ClampMin=0, Units="cm"))
	float ExplosionRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile|Physics", meta=(ClampMin=0))
	float PhysicsImpulse = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(ClampMin=0, Units="s"))
	float MaxLifeSeconds = 10.0f;

	/** Keeps an impacted projectile alive briefly so its reliable impact RPC can reach clients. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(ClampMin=0.05, Units="s"))
	float ImpactDestroyDelay = 0.2f;

private:
	bool ApplyImpactDamage(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit);

	UPROPERTY()
	TObjectPtr<ANetProjectileWeapon> SourceWeapon;

	bool bHit = false;
};
