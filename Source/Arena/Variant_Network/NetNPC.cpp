// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetNPC.h"
#include "Arena.h"
#include "AI/NetAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/NetHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NetGameMode.h"

ANetNPC::ANetNPC()
{
	bReplicates = true;
	AIControllerClass = ANetAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WeaponTrace, ECR_Block);
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

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

	if (Damage > 0.0f)
	{
		LastDamageInstigator = EventInstigator;
		if (!LastDamageInstigator.IsValid() && DamageCauser)
		{
			LastDamageInstigator = DamageCauser->GetInstigatorController();
		}
	}

	return HealthComponent->ApplyHealthDamage(Damage);
}

bool ANetNPC::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

bool ANetNPC::TryAttack(AActor* Target)
{
	if (!HasAuthority() || IsDead() || !IsValid(Target))
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < AttackCooldown)
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (DistanceToTarget > AttackRange)
	{
		return false;
	}

	LastAttackTime = CurrentTime;
	UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, nullptr);

	return true;
}

void ANetNPC::OnDeath()
{
	GetCharacterMovement()->DisableMovement();

	if (HasAuthority())
	{
		if (ANetGameMode* NetGameMode = GetWorld()->GetAuthGameMode<ANetGameMode>())
		{
			NetGameMode->NotifyEnemyKilled(this, LastDamageInstigator.Get());
		}

		Destroy();
	}
}
