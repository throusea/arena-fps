// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetWeaponPickup.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "NetWeaponHolder.h"
#include "NetRifle.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANetWeaponPickup::ANetWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collision"));
	SphereCollision->SetupAttachment(RootComponent);
	SphereCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 84.0f));
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionObjectType(ECC_WorldStatic);
	SphereCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollision->bFillCollisionUnderneathForNavmesh = true;
	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &ANetWeaponPickup::OnOverlap);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SphereCollision);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
}

void ANetWeaponPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LoadWeaponData();
}

void ANetWeaponPickup::BeginPlay()
{
	Super::BeginPlay();

	LoadWeaponData();
	ApplyPickupState(bIsAvailable);

	if (HasAuthority() && !WeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon pickup '%s' has no valid weapon class."), *GetNameSafe(this));
	}
}

void ANetWeaponPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	Super::EndPlay(EndPlayReason);
}

void ANetWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANetWeaponPickup, bIsAvailable);
}

void ANetWeaponPickup::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsAvailable || !WeaponClass || !IsValid(OtherActor))
	{
		return;
	}

	INetWeaponHolder* WeaponHolder = Cast<INetWeaponHolder>(OtherActor);
	if (!WeaponHolder)
	{
		return;
	}

	// Mark unavailable before granting the weapon so simultaneous overlaps cannot consume it twice.
	SetPickupAvailable(false);
	WeaponHolder->AddWeaponClass(WeaponClass);
	MulticastOnCollected(OtherActor);

	if (RespawnTime <= 0.0f)
	{
		RespawnPickup();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			RespawnTimer, this, &ANetWeaponPickup::RespawnPickup, RespawnTime, false);
	}
}

void ANetWeaponPickup::OnRep_IsAvailable(bool bWasAvailable)
{
	ApplyPickupState(bWasAvailable);
}

void ANetWeaponPickup::MulticastOnCollected_Implementation(AActor* CollectingActor)
{
	BP_OnCollected(CollectingActor);
}

void ANetWeaponPickup::RespawnPickup()
{
	if (HasAuthority())
	{
		SetPickupAvailable(true);
	}
}

void ANetWeaponPickup::SetPickupAvailable(bool bNewAvailable)
{
	if (!HasAuthority() || bIsAvailable == bNewAvailable)
	{
		return;
	}

	const bool bWasAvailable = bIsAvailable;
	bIsAvailable = bNewAvailable;
	ApplyPickupState(bWasAvailable);
	ForceNetUpdate();
}

void ANetWeaponPickup::ApplyPickupState(bool bWasAvailable)
{
	SetActorHiddenInGame(!bIsAvailable);
	SphereCollision->SetCollisionEnabled(
		HasAuthority() && bIsAvailable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	if (!bWasAvailable && bIsAvailable)
	{
		BP_OnRespawn();
	}
}

void ANetWeaponPickup::LoadWeaponData()
{
	if (const FNetWeaponPickupTableRow* WeaponData =
		WeaponType.GetRow<FNetWeaponPickupTableRow>(GetNameSafe(this)))
	{
		Mesh->SetStaticMesh(WeaponData->StaticMesh.LoadSynchronous());
		WeaponClass = WeaponData->WeaponToSpawn;
	}
	else
	{
		Mesh->SetStaticMesh(nullptr);
		WeaponClass = nullptr;
	}
}
