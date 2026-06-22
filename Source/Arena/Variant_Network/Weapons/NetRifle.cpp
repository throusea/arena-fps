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
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANetWeaponBase::ANetWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
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

void ANetWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	WeaponOwner = Cast<INetWeaponHolder>(GetOwner());
	PawnOwner = Cast<APawn>(GetOwner());
	SetActorTickEnabled(HasAuthority());
	CurrentSpreadAngle = GetEffectiveMinSpreadAngle();

	if (HasAuthority())
	{
		CurrentAmmo = MagazineSize;
		UpdateSpread(0.0f);
	}

	BroadcastAmmoChanged();
	BroadcastReloadStateChanged();
	BroadcastSpreadChanged();
}

void ANetWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		UpdateSpread(DeltaSeconds);
	}
}

void ANetWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetWeaponBase, LastHitResult);
	DOREPLIFETIME(ANetWeaponBase, MagazineSize);
	DOREPLIFETIME(ANetWeaponBase, CurrentAmmo);
	DOREPLIFETIME(ANetWeaponBase, ReloadState);
	DOREPLIFETIME_CONDITION(ANetWeaponBase, CurrentSpreadAngle, COND_OwnerOnly);
}

void ANetWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireCooldownTimer);
		World->GetTimerManager().ClearTimer(ReloadTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ANetWeaponBase::StartFiringOnServer()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bWantsToFire)
	{
		return;
	}

	bWantsToFire = true;
	FireOnServer();
}

void ANetWeaponBase::StopFiringOnServer()
{
	if (!HasAuthority())
	{
		return;
	}

	bWantsToFire = false;
}

void ANetWeaponBase::MulticastPlayFireEffects_Implementation(const FNetWeaponFireResult& FireResult)
{
	if (!HasAuthority() && PawnOwner && PawnOwner->IsLocallyControlled())
	{
		SetCurrentSpreadAngle(FireResult.SpreadAngle);
	}

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
		: FVector(FireResult.Origin);
	const FVector ShotDirection = FVector(FireResult.Direction).GetSafeNormal();
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

	BP_OnWeaponFired(FireResult);
	OnWeaponFired.Broadcast(FireResult);
}

void ANetWeaponBase::MulticastPlayHitEffects_Implementation(const FNetWeaponImpactResult& ImpactResult)
{
	if (HasAuthority() && ImpactResult.bDamagedActor)
	{
		ClientConfirmHit(ImpactResult);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ImpactResult.ImpactLocation);
	}

	BP_OnWeaponHit(ImpactResult);
	OnWeaponHit.Broadcast(ImpactResult);
}

void ANetWeaponBase::ClientConfirmHit_Implementation(const FNetWeaponImpactResult& ImpactResult)
{
	if (WeaponOwner)
	{
		WeaponOwner->ReportWeaponHitConfirmed(ImpactResult);
	}
}

void ANetWeaponBase::FireOnServer()
{
	if (!bWantsToFire || !bCanFire || ReloadState.bIsReloading || !CanFireAuthoritativeShot())
	{
		return;
	}

	if (bConsumeAmmo && CurrentAmmo <= 0)
	{
		StartReload();
		return;
	}

	bCanFire = false;
	LastHitResult = FHitResult();
	IncreaseSpreadForShot();

	FNetWeaponFireResult FireResult;
	FNetWeaponImpactResult ImpactResult;
	if (!FireAuthoritativeShot(FireResult, ImpactResult))
	{
		bCanFire = true;
		return;
	}

	MulticastPlayFireEffects(FireResult);
	if (ImpactResult.bBlockingHit)
	{
		MulticastPlayHitEffects(ImpactResult);
	}
	if (bConsumeAmmo)
	{
		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
		BroadcastAmmoChanged();

		if (CurrentAmmo <= 0)
		{
			StartReload();
		}
	}

	const float EffectiveCooldown = FMath::Max(FireCooldown, 0.01f);
	GetWorld()->GetTimerManager().SetTimer(FireCooldownTimer, this, &ANetWeaponBase::FireCooldownExpired, EffectiveCooldown, false);
}

void ANetWeaponBase::FireCooldownExpired()
{
	bCanFire = true;

	if (!bFullAuto && WeaponOwner)
	{
		WeaponOwner->OnSemiWeaponRefire();
	}

	if (bWantsToFire && bFullAuto)
	{
		FireOnServer();
	}
}

bool ANetWeaponBase::CanFireAuthoritativeShot() const
{
	return WeaponOwner != nullptr && IsValid(PawnOwner) && IsValid(GetWorld());
}

bool ANetWeaponBase::CalculateShotPath(FVector& OutStart, FVector& OutDirection, FVector& OutEnd) const
{
	if (!WeaponOwner || !PawnOwner)
	{
		return false;
	}

	FRotator TraceRotation;
	if (AController* Controller = PawnOwner->GetController())
	{
		Controller->GetPlayerViewPoint(OutStart, TraceRotation);
	}
	else
	{
		PawnOwner->GetActorEyesViewPoint(OutStart, TraceRotation);
	}

	const FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
	OutDirection = (TargetLocation - OutStart).GetSafeNormal();
	if (OutDirection.IsNearlyZero())
	{
		OutDirection = TraceRotation.Vector();
	}

	if (CurrentSpreadAngle > 0.0f)
	{
		OutDirection = FMath::VRandCone(OutDirection, FMath::DegreesToRadians(CurrentSpreadAngle));
	}

	OutEnd = OutStart + (OutDirection * TraceDistance);
	return true;
}

bool ANetRifle::FireAuthoritativeShot(FNetWeaponFireResult& OutFireResult, FNetWeaponImpactResult& OutImpactResult)
{
	LastHitResult = FHitResult();

	FVector TraceStart;
	FVector ShotDirection;
	FVector TraceEnd;
	if (!CalculateShotPath(TraceStart, ShotDirection, TraceEnd))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NetRifleTrace), true, PawnOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(PawnOwner);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(LastHitResult, TraceStart, TraceEnd, TraceChannel, QueryParams);
	LastHitResult.TraceStart = TraceStart;
	LastHitResult.TraceEnd = TraceEnd;
	const FVector ShotEnd = bHit ? LastHitResult.ImpactPoint : TraceEnd;

	OutFireResult.Origin = TraceStart;
	OutFireResult.Direction = ShotDirection;
	OutFireResult.SpreadAngle = CurrentSpreadAngle;

	if (bHit)
	{
		OutImpactResult.ImpactLocation = LastHitResult.ImpactPoint;
		OutImpactResult.ImpactNormal = LastHitResult.ImpactNormal;
		OutImpactResult.HitActor = LastHitResult.GetActor();
		OutImpactResult.bBlockingHit = true;

		if (AActor* HitActor = LastHitResult.GetActor())
		{
			AController* InstigatorController = PawnOwner ? PawnOwner->GetController() : nullptr;
			const float AppliedDamage = UGameplayStatics::ApplyPointDamage(
				HitActor,
				Damage,
				ShotDirection,
				LastHitResult,
				InstigatorController,
				this,
				nullptr);
			OutImpactResult.bDamagedActor = AppliedDamage > 0.0f;
		}
	}

	if (GEngine)
	{
		const FString HitActorName = bHit ? GetNameSafe(LastHitResult.GetActor()) : TEXT("None");
		GEngine->AddOnScreenDebugMessage(7, 1.0f, bHit ? FColor::Red : FColor::Yellow, FString::Printf(TEXT("Server rifle fired. Hit: %s"), *HitActorName));
	}

	if (DebugTraceDuration > 0.0f)
		DrawDebugLine(GetWorld(), TraceStart, ShotEnd, bHit ? FColor::Red : FColor::Yellow, false, DebugTraceDuration, 0, 1.0f);

	return true;
}

void ANetWeaponBase::OnRep_CurrentAmmo()
{
	BroadcastAmmoChanged();
}

void ANetWeaponBase::OnRep_ReloadState()
{
	BroadcastReloadStateChanged();
}

void ANetWeaponBase::OnRep_CurrentSpreadAngle()
{
	BroadcastSpreadChanged();
}

void ANetWeaponBase::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
	BP_OnAmmoChanged(CurrentAmmo, MagazineSize);

	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentAmmo, MagazineSize);
	}
}

void ANetWeaponBase::BroadcastReloadStateChanged()
{
	OnReloadStateChanged.Broadcast(ReloadState.bIsReloading);
	BP_OnReloadStateChanged(ReloadState.bIsReloading);
}

void ANetWeaponBase::StartReload()
{
	if (!HasAuthority() || !bConsumeAmmo || ReloadState.bIsReloading || CurrentAmmo > 0)
	{
		return;
	}

	const float EffectiveReloadDuration = FMath::Max(ReloadDuration, 0.01f);
	ReloadState.bIsReloading = true;
	ReloadState.ReloadEndServerTime = GetSynchronizedWorldTime() + EffectiveReloadDuration;
	BroadcastReloadStateChanged();
	ForceNetUpdate();

	GetWorldTimerManager().SetTimer(
		ReloadTimer, this, &ANetWeaponBase::FinishReload, EffectiveReloadDuration, false);
}

void ANetWeaponBase::FinishReload()
{
	if (!HasAuthority() || !ReloadState.bIsReloading)
	{
		return;
	}

	CurrentAmmo = MagazineSize;
	ReloadState.bIsReloading = false;
	ReloadState.ReloadEndServerTime = 0.0f;
	BroadcastAmmoChanged();
	BroadcastReloadStateChanged();
	ForceNetUpdate();

	if (bWantsToFire && bFullAuto && bCanFire)
	{
		FireOnServer();
	}
}

float ANetWeaponBase::GetReloadProgress() const
{
	if (!ReloadState.bIsReloading)
	{
		return 0.0f;
	}

	const float EffectiveReloadDuration = FMath::Max(ReloadDuration, 0.01f);
	return FMath::Clamp(1.0f - (GetReloadRemainingTime() / EffectiveReloadDuration), 0.0f, 1.0f);
}

float ANetWeaponBase::GetReloadRemainingTime() const
{
	return ReloadState.bIsReloading
		? FMath::Max(0.0f, ReloadState.ReloadEndServerTime - GetSynchronizedWorldTime())
		: 0.0f;
}

float ANetWeaponBase::GetSynchronizedWorldTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	if (const AGameStateBase* GameState = World->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return World->GetTimeSeconds();
}

float ANetWeaponBase::GetNormalizedSpread() const
{
	const float EffectiveMinSpread = GetEffectiveMinSpreadAngle();
	const float SpreadRange = GetEffectiveMaxSpreadAngle() - EffectiveMinSpread;
	return SpreadRange > KINDA_SMALL_NUMBER
		? FMath::Clamp((CurrentSpreadAngle - EffectiveMinSpread) / SpreadRange, 0.0f, 1.0f)
		: 0.0f;
}

void ANetWeaponBase::BroadcastSpreadChanged()
{
	const float NormalizedSpread = GetNormalizedSpread();
	OnSpreadChanged.Broadcast(CurrentSpreadAngle, NormalizedSpread);
	BP_OnSpreadChanged(CurrentSpreadAngle, NormalizedSpread);
}

void ANetWeaponBase::IncreaseSpreadForShot()
{
	FiringSpread = FMath::Clamp(
		FiringSpread + SpreadIncreasePerShot,
		0.0f,
		GetEffectiveMaxSpreadAngle() - GetEffectiveMinSpreadAngle());
	UpdateSpread(0.0f);
	ForceNetUpdate();
}

void ANetWeaponBase::UpdateSpread(float DeltaSeconds)
{
	const bool bMagazineEmpty = bConsumeAmmo && CurrentAmmo <= 0;
	const bool bShouldRecover = !bWantsToFire || !bFullAuto || bMagazineEmpty;
	if (bShouldRecover && SpreadRecoverySpeed > 0.0f)
	{
		FiringSpread = FMath::FInterpTo(FiringSpread, 0.0f, DeltaSeconds, SpreadRecoverySpeed);
		if (FiringSpread < KINDA_SMALL_NUMBER)
		{
			FiringSpread = 0.0f;
		}
	}

	SetCurrentSpreadAngle(CalculateStateSpread() + FiringSpread);
}

float ANetWeaponBase::CalculateStateSpread() const
{
	float StateSpread = GetEffectiveMinSpreadAngle();
	if (!PawnOwner)
	{
		return StateSpread;
	}

	const float HorizontalSpeed = PawnOwner->GetVelocity().Size2D();
	if (HorizontalSpeed > MovingSpeedThreshold)
	{
		const UPawnMovementComponent* MovementComponent = PawnOwner->GetMovementComponent();
		const float MaxMovementSpeed = MovementComponent ? MovementComponent->GetMaxSpeed() : HorizontalSpeed;
		const float MovementAlpha = MaxMovementSpeed > MovingSpeedThreshold
			? FMath::Clamp((HorizontalSpeed - MovingSpeedThreshold) / (MaxMovementSpeed - MovingSpeedThreshold), 0.0f, 1.0f)
			: 1.0f;
		StateSpread += MovementSpread * MovementAlpha;
	}

	if (const ACharacter* CharacterOwner = Cast<ACharacter>(PawnOwner))
	{
		if (const UCharacterMovementComponent* CharacterMovement = CharacterOwner->GetCharacterMovement();
			CharacterMovement && CharacterMovement->IsFalling())
		{
			StateSpread += AirborneSpread;
		}

	}

	return StateSpread;
}

void ANetWeaponBase::SetCurrentSpreadAngle(float NewSpreadAngle)
{
	const float ClampedSpread = FMath::Clamp(
		NewSpreadAngle,
		GetEffectiveMinSpreadAngle(),
		GetEffectiveMaxSpreadAngle());
	if (FMath::IsNearlyEqual(CurrentSpreadAngle, ClampedSpread, 0.001f))
	{
		return;
	}

	CurrentSpreadAngle = ClampedSpread;
	BroadcastSpreadChanged();
}
