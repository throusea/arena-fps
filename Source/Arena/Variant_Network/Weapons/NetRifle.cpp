// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetRifle.h"
#include "Variant_Network/NetCharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANetRifle::ANetRifle()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANetRifle::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ANetCharacter>(GetOwner());
}

void ANetRifle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireCooldownTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ANetRifle::StartFiring()
{
	bWantsToFire = true;
	Fire();
}

void ANetRifle::StopFiring()
{
	bWantsToFire = false;
}

void ANetRifle::Fire()
{
	if (!bWantsToFire || !bCanFire)
	{
		return;
	}

	bCanFire = false;
	LastHitResult = FHitResult();

	const bool bHit = TraceFromCamera(LastHitResult);
	const FVector TraceEnd = bHit ? LastHitResult.ImpactPoint : LastHitResult.TraceEnd;

	if (GEngine)
	{
		const FString HitActorName = bHit ? GetNameSafe(LastHitResult.GetActor()) : TEXT("None");
		GEngine->AddOnScreenDebugMessage(7, 1.0f, bHit ? FColor::Red : FColor::Yellow, FString::Printf(TEXT("Rifle fired. Hit: %s"), *HitActorName));
	}

	DrawDebugLine(GetWorld(), LastHitResult.TraceStart, TraceEnd, bHit ? FColor::Red : FColor::Yellow, false, 1.0f, 0, 1.0f);

	GetWorld()->GetTimerManager().SetTimer(FireCooldownTimer, this, &ANetRifle::FireCooldownExpired, FireCooldown, false);
}

void ANetRifle::FireCooldownExpired()
{
	bCanFire = true;

	if (bWantsToFire)
	{
		Fire();
	}
}

bool ANetRifle::TraceFromCamera(FHitResult& OutHitResult) const
{
	if (!OwningCharacter)
	{
		return false;
	}

	const UCameraComponent* Camera = OwningCharacter->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return false;
	}

	const FVector TraceStart = Camera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NetRifleTrace), true, OwningCharacter);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningCharacter);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, QueryParams);
	OutHitResult.TraceStart = TraceStart;
	OutHitResult.TraceEnd = TraceEnd;

	return bHit;
}
