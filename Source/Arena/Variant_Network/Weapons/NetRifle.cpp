// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetRifle.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANetRifle::ANetRifle()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(RootComponent);
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetOnlyOwnerSee(true);

	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonMesh"));
	ThirdPersonMesh->SetupAttachment(RootComponent);
	ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonMesh->SetOwnerNoSee(true);
}

void ANetRifle::BeginPlay()
{
	Super::BeginPlay();

	WeaponOwner = Cast<INetWeaponHolder>(GetOwner());
	PawnOwner = Cast<APawn>(GetOwner());

	if (HasAuthority())
	{
		CurrentAmmo = MagazineSize;
	}

	BroadcastAmmoChanged();
}

void ANetRifle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetRifle, LastHitResult);
	DOREPLIFETIME(ANetRifle, MagazineSize);
	DOREPLIFETIME(ANetRifle, CurrentAmmo);
}

void ANetRifle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireCooldownTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ANetRifle::StartFiringOnServer()
{
	if (!HasAuthority())
	{
		return;
	}

	bWantsToFire = true;
	FireOnServer();
}

void ANetRifle::StopFiringOnServer()
{
	if (!HasAuthority())
	{
		return;
	}

	bWantsToFire = false;
}

void ANetRifle::MulticastPlayFireEffects_Implementation(const FNetWeaponShotResult& ShotResult)
{
	if (WeaponOwner)
	{
		WeaponOwner->PlayFiringMontage(FiringMontage);
		WeaponOwner->AddWeaponRecoil(FiringRecoil);
	}

	if (PawnOwner && PawnOwner->IsLocallyControlled() && FiringCameraShake)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(PawnOwner->GetController()))
		{
			if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
			{
				CameraManager->StartCameraShake(FiringCameraShake, CameraShakeScale);
			}
		}
	}

	USkeletalMeshComponent* MuzzleMesh = PawnOwner && PawnOwner->IsLocallyControlled()
		? FirstPersonMesh
		: ThirdPersonMesh;
	const bool bHasMuzzleSocket = MuzzleMesh && MuzzleMesh->DoesSocketExist(MuzzleSocketName);
	const FVector MuzzleLocation = bHasMuzzleSocket
		? MuzzleMesh->GetSocketLocation(MuzzleSocketName)
		: FVector(ShotResult.TraceStart);
	const FVector ShotDirection = (FVector(ShotResult.TraceEnd) - MuzzleLocation).GetSafeNormal();
	const FVector EffectLocation = MuzzleLocation + (ShotDirection * MuzzleOffset);
	const FRotator ShotRotation = ShotDirection.IsNearlyZero()
		? FRotator::ZeroRotator
		: FRotationMatrix::MakeFromZ(ShotDirection).Rotator();

	if (MuzzleFlashFX)
	{
		if (bHasMuzzleSocket)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				MuzzleFlashFX,
				MuzzleMesh,
				MuzzleSocketName,
				EffectLocation,
				ShotRotation,
				EAttachLocation::KeepWorldPosition,
				true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, MuzzleFlashFX, EffectLocation, ShotRotation);
		}
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, EffectLocation);
	}

	BP_OnWeaponFired(ShotResult);
	OnWeaponFired.Broadcast(ShotResult);
}

void ANetRifle::MulticastPlayHitEffects_Implementation(const FNetWeaponShotResult& ShotResult)
{
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ShotResult.TraceEnd);
	}

	BP_OnWeaponHit(ShotResult);
	OnWeaponHit.Broadcast(ShotResult);
}

void ANetRifle::ReportProjectileHit(const FHitResult& HitResult, bool bDamagedActor)
{
	if (!HasAuthority() || !HitResult.bBlockingHit)
	{
		return;
	}

	LastHitResult = HitResult;

	FNetWeaponShotResult ShotResult;
	ShotResult.TraceStart = HitResult.TraceStart;
	ShotResult.TraceEnd = HitResult.ImpactPoint;
	ShotResult.ImpactNormal = HitResult.ImpactNormal;
	ShotResult.HitActor = HitResult.GetActor();
	ShotResult.bBlockingHit = true;
	ShotResult.bDamagedActor = bDamagedActor;
	MulticastPlayHitEffects(ShotResult);
}

void ANetRifle::FireOnServer()
{
	if (!bWantsToFire || !bCanFire || (bConsumeAmmo && CurrentAmmo <= 0))
	{
		return;
	}

	bCanFire = false;
	LastHitResult = FHitResult();

	const FNetWeaponShotResult ShotResult = FireAuthoritativeShot();
	MulticastPlayFireEffects(ShotResult);
	if (ShotResult.bBlockingHit)
	{
		MulticastPlayHitEffects(ShotResult);
	}
	if (bConsumeAmmo)
	{
		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
		BroadcastAmmoChanged();
	}

	GetWorld()->GetTimerManager().SetTimer(FireCooldownTimer, this, &ANetRifle::FireCooldownExpired, FireCooldown, false);
}

void ANetRifle::FireCooldownExpired()
{
	bCanFire = true;

	if (WeaponOwner)
	{
		WeaponOwner->OnSemiWeaponRefire();
	}

	if (bWantsToFire && bFullAuto)
	{
		FireOnServer();
	}
}

bool ANetRifle::TraceFromCamera(FHitResult& OutHitResult) const
{
	if (!WeaponOwner || !PawnOwner)
	{
		return false;
	}

	FVector TraceStart;
	FRotator TraceRotation;
	if (AController* Controller = PawnOwner->GetController())
	{
		Controller->GetPlayerViewPoint(TraceStart, TraceRotation);
	}
	else
	{
		PawnOwner->GetActorEyesViewPoint(TraceStart, TraceRotation);
	}

	const FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
	FVector ShotDirection = (TargetLocation - TraceStart).GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		ShotDirection = TraceRotation.Vector();
	}

	if (SpreadHalfAngle > 0.0f)
	{
		ShotDirection = FMath::VRandCone(ShotDirection, FMath::DegreesToRadians(SpreadHalfAngle));
	}

	const FVector TraceEnd = TraceStart + (ShotDirection * TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NetRifleTrace), true, PawnOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(PawnOwner);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, QueryParams);
	OutHitResult.TraceStart = TraceStart;
	OutHitResult.TraceEnd = TraceEnd;

	return bHit;
}

FNetWeaponShotResult ANetRifle::FireAuthoritativeShot()
{
	LastHitResult = FHitResult();

	const bool bHit = TraceFromCamera(LastHitResult);
	const FVector TraceEnd = bHit ? LastHitResult.ImpactPoint : LastHitResult.TraceEnd;

	FNetWeaponShotResult ShotResult;
	ShotResult.TraceStart = LastHitResult.TraceStart;
	ShotResult.TraceEnd = TraceEnd;
	ShotResult.ImpactNormal = bHit ? LastHitResult.ImpactNormal : FVector::ZeroVector;
	ShotResult.HitActor = bHit ? LastHitResult.GetActor() : nullptr;
	ShotResult.bBlockingHit = bHit && FireType == ENetWeaponFireType::Hitscan;

	if (FireType == ENetWeaponFireType::Projectile)
	{
		FireProjectileOnServer(ShotResult);
		LastHitResult = FHitResult();
		return ShotResult;
	}

	if (bHit)
	{
		if (AActor* HitActor = LastHitResult.GetActor())
		{
			AController* InstigatorController = PawnOwner ? PawnOwner->GetController() : nullptr;
			const float AppliedDamage = UGameplayStatics::ApplyDamage(HitActor, Damage, InstigatorController, this, nullptr);
			ShotResult.bDamagedActor = AppliedDamage > 0.0f;
		}
	}

	if (GEngine)
	{
		const FString HitActorName = bHit ? GetNameSafe(LastHitResult.GetActor()) : TEXT("None");
		GEngine->AddOnScreenDebugMessage(7, 1.0f, bHit ? FColor::Red : FColor::Yellow, FString::Printf(TEXT("Server rifle fired. Hit: %s"), *HitActorName));
	}

	DrawDebugLine(GetWorld(), LastHitResult.TraceStart, TraceEnd, bHit ? FColor::Red : FColor::Yellow, false, DebugTraceDuration, 0, 1.0f);

	return ShotResult;
}

void ANetRifle::FireProjectileOnServer(const FNetWeaponShotResult& ShotResult)
{
	if (!ProjectileClass)
	{
		return;
	}

	const FVector ShotDirection = (ShotResult.TraceEnd - ShotResult.TraceStart).GetSafeNormal();
	const FVector SpawnLocation = ShotResult.TraceStart + (ShotDirection * MuzzleOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = PawnOwner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnLocation, ShotDirection.Rotation(), SpawnParams);
}

void ANetRifle::OnRep_CurrentAmmo()
{
	BroadcastAmmoChanged();
}

void ANetRifle::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
	BP_OnAmmoChanged(CurrentAmmo, MagazineSize);

	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentAmmo, MagazineSize);
	}
}
