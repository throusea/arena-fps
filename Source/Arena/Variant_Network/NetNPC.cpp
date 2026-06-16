// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetNPC.h"
#include "Arena.h"
#include "Components/CapsuleComponent.h"
#include "Components/NetHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ANetNPC::ANetNPC()
{
	bReplicates = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WeaponTrace, ECR_Block);

	HealthComponent = CreateDefaultSubobject<UNetHealthComponent>(TEXT("Health Component"));
}

void ANetNPC::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ANetNPC::OnDeath);
	}
}

float ANetNPC::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || !HealthComponent)
	{
		return AppliedDamage;
	}

	return HealthComponent->ApplyHealthDamage(Damage);
}

bool ANetNPC::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

void ANetNPC::OnDeath()
{
	GetCharacterMovement()->DisableMovement();

	if (HasAuthority())
	{
		Destroy();
	}
}
