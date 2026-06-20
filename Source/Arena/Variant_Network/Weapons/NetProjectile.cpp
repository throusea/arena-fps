// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetProjectile.h"
#include "NetProjectileWeapon.h"
#include "Components/SphereComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ANetProjectile::ANetProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	DamageType = UDamageType::StaticClass();
}

void ANetProjectile::InitializeProjectile(ANetProjectileWeapon* InSourceWeapon)
{
	if (HasAuthority())
	{
		SourceWeapon = InSourceWeapon;
		CollisionComponent->IgnoreActorWhenMoving(SourceWeapon, true);

		if (AActor* InstigatorActor = GetInstigator())
		{
			CollisionComponent->IgnoreActorWhenMoving(InstigatorActor, true);
		}
	}
}

void ANetProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && MaxLifeSeconds > 0.0f)
	{
		SetLifeSpan(MaxLifeSeconds);
	}
}

void ANetProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse,
	const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (!HasAuthority() || bHit)
	{
		return;
	}
	bHit = true;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();

	const bool bDamagedActor = ApplyImpactDamage(Other, OtherComp, Hit);
	if (IsValid(SourceWeapon))
	{
		SourceWeapon->HandleProjectileImpact(Hit, bDamagedActor);
	}

	MulticastHandleImpact(Hit.ImpactPoint, Hit.ImpactNormal, bExplodeOnImpact);
	SetLifeSpan(FMath::Max(ImpactDestroyDelay, 0.05f));
}

void ANetProjectile::MulticastHandleImpact_Implementation(
	FVector_NetQuantize ImpactLocation,
	FVector_NetQuantizeNormal ImpactNormal,
	bool bExploded)
{
	SetActorHiddenInGame(true);
	BP_OnProjectileImpact(ImpactLocation, ImpactNormal, bExploded);
}

bool ANetProjectile::ApplyImpactDamage(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit)
{
	AController* InstigatorController = GetInstigatorController();
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);
	if (!bDamageInstigator && GetInstigator())
	{
		IgnoredActors.Add(GetInstigator());
	}

	bool bDamagedActor = false;
	if (bExplodeOnImpact)
	{
		bDamagedActor = UGameplayStatics::ApplyRadialDamage(
			this,
			ExplosionDamage,
			Hit.ImpactPoint,
			ExplosionRadius,
			DamageType,
			IgnoredActors,
			this,
			InstigatorController,
			false,
			ECC_Visibility);
	}
	else if (IsValid(HitActor) && (bDamageInstigator || HitActor != GetInstigator()))
	{
		bDamagedActor = UGameplayStatics::ApplyPointDamage(
			HitActor,
			HitDamage,
			-Hit.ImpactNormal.GetSafeNormal(),
			Hit,
			InstigatorController,
			this,
			DamageType) > 0.0f;
	}

	if (HitComponent && HitComponent->IsSimulatingPhysics() && PhysicsImpulse > 0.0f)
	{
		const FVector ImpulseDirection = -Hit.ImpactNormal.GetSafeNormal();
		HitComponent->AddImpulseAtLocation(ImpulseDirection * PhysicsImpulse, Hit.ImpactPoint);
	}

	return bDamagedActor;
}
