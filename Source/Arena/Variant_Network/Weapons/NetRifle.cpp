// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetRifle.h"
#include "Variant_Network/NetCharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANetRifle::ANetRifle()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ANetRifle::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ANetCharacter>(GetOwner());

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
	bWantsToFire = true;
	FireOnServer();
}

void ANetRifle::StopFiringOnServer()
{
	bWantsToFire = false;
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

	if (bWantsToFire)
	{
		FireOnServer();
	}
}

bool ANetRifle::TraceFromCamera(FHitResult& OutHitResult) const
{
	if (!OwningCharacter)
	{
		return false;
	}

	FVector TraceStart;
	FRotator TraceRotation;
	if (AController* Controller = OwningCharacter->GetController())
	{
		Controller->GetPlayerViewPoint(TraceStart, TraceRotation);
	}
	else
	{
		const UCameraComponent* Camera = OwningCharacter->GetFirstPersonCameraComponent();
		if (!Camera)
		{
			return false;
		}

		TraceStart = Camera->GetComponentLocation();
		TraceRotation = Camera->GetComponentRotation();
	}

	const FVector TraceEnd = TraceStart + (TraceRotation.Vector() * TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NetRifleTrace), true, OwningCharacter);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningCharacter);

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
			AController* InstigatorController = OwningCharacter ? OwningCharacter->GetController() : nullptr;
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
}
