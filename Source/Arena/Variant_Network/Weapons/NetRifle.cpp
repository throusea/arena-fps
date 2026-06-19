// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetRifle.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
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

void ANetRifle::MulticastPlayFireEffects_Implementation(
	FVector_NetQuantize TraceStart,
	FVector_NetQuantize TraceEnd,
	bool bBlockingHit)
{
	if (WeaponOwner)
	{
		WeaponOwner->PlayFiringMontage(FiringMontage);
		WeaponOwner->AddWeaponRecoil(FiringRecoil);
	}

	USkeletalMeshComponent* MuzzleMesh = PawnOwner && PawnOwner->IsLocallyControlled()
		? FirstPersonMesh
		: ThirdPersonMesh;
	const bool bHasMuzzleSocket = MuzzleMesh && MuzzleMesh->DoesSocketExist(MuzzleSocketName);
	const FVector MuzzleLocation = bHasMuzzleSocket
		? MuzzleMesh->GetSocketLocation(MuzzleSocketName)
		: TraceStart;
	const FVector ShotDirection = (TraceEnd - MuzzleLocation).GetSafeNormal();
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

	if (bBlockingHit && ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, TraceEnd);
	}
}

void ANetRifle::FireOnServer()
{
	if (!bWantsToFire || !bCanFire || (bConsumeAmmo && CurrentAmmo <= 0))
	{
		return;
	}

	bCanFire = false;
	LastHitResult = FHitResult();

	FireAuthoritativeShot();
	const FVector ShotEnd = LastHitResult.bBlockingHit ? LastHitResult.ImpactPoint : LastHitResult.TraceEnd;
	MulticastPlayFireEffects(LastHitResult.TraceStart, ShotEnd, LastHitResult.bBlockingHit);
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

	FVector TraceEnd = WeaponOwner->GetWeaponTargetLocation();
	if (TraceEnd.Equals(TraceStart))
	{
		TraceEnd = TraceStart + (TraceRotation.Vector() * TraceDistance);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NetRifleTrace), true, PawnOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(PawnOwner);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, QueryParams);
	OutHitResult.TraceStart = TraceStart;
	OutHitResult.TraceEnd = TraceEnd;

	return bHit;
}

void ANetRifle::FireAuthoritativeShot()
{
	LastHitResult = FHitResult();

	const bool bHit = TraceFromCamera(LastHitResult);
	const FVector TraceEnd = bHit ? LastHitResult.ImpactPoint : LastHitResult.TraceEnd;

	if (bHit)
	{
		if (AActor* HitActor = LastHitResult.GetActor())
		{
			AController* InstigatorController = PawnOwner ? PawnOwner->GetController() : nullptr;
			UGameplayStatics::ApplyDamage(HitActor, Damage, InstigatorController, this, nullptr);
		}
	}

	if (GEngine)
	{
		const FString HitActorName = bHit ? GetNameSafe(LastHitResult.GetActor()) : TEXT("None");
		GEngine->AddOnScreenDebugMessage(7, 1.0f, bHit ? FColor::Red : FColor::Yellow, FString::Printf(TEXT("Server rifle fired. Hit: %s"), *HitActorName));
	}

	DrawDebugLine(GetWorld(), LastHitResult.TraceStart, TraceEnd, bHit ? FColor::Red : FColor::Yellow, false, DebugTraceDuration, 0, 1.0f);
}

void ANetRifle::OnRep_CurrentAmmo()
{
	BroadcastAmmoChanged();
}

void ANetRifle::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);

	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentAmmo, MagazineSize);
	}
}
