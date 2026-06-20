// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetNPC.h"
#include "Arena.h"
#include "AI/NetAIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/NetHealthComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NetGameMode.h"
#include "TimerManager.h"

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

void ANetNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AttackWindupTimerHandle);
	Super::EndPlay(EndPlayReason);
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

	const float HealthDamage = HealthComponent->ApplyHealthDamage(Damage);
	if (HealthDamage <= 0.0f)
	{
		return 0.0f;
	}

	const FNetNPCHitPresentation HitPresentation = BuildHitPresentation(
		HealthDamage,
		DamageEvent,
		EventInstigator,
		DamageCauser);
	MulticastPlayHitPresentation(HitPresentation);

	if (!HitPresentation.bFatal)
	{
		ApplyHitKnockback(HitPresentation.HitDirection);
	}

	return HealthDamage;
}

bool ANetNPC::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

bool ANetNPC::TryAttack(AActor* Target)
{
	if (!HasAuthority() || IsDead() || bAttackInProgress || !IsValid(Target))
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
	bAttackInProgress = true;
	MulticastPlayAttackPresentation(Target);

	if (AttackWindup <= 0.0f)
	{
		DoAttack(Target);
	}
	else
	{
		const TWeakObjectPtr<AActor> WeakTarget = Target;
		FTimerDelegate AttackDelegate;
		AttackDelegate.BindWeakLambda(
			this,
			[this, WeakTarget]()
			{
				DoAttack(WeakTarget.Get());
			});
		GetWorldTimerManager().SetTimer(AttackWindupTimerHandle, AttackDelegate, AttackWindup, false);
	}

	return true;
}

void ANetNPC::DoAttack(AActor* Target)
{
	if (!bAttackInProgress)
	{
		return;
	}

	bAttackInProgress = false;
	if (!HasAuthority() || IsDead() || !IsValid(Target))
	{
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (DistanceToTarget > AttackRange)
	{
		return;
	}

	UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, nullptr);
}
void ANetNPC::MulticastPlayAttackPresentation_Implementation(AActor* Target)
{
	BP_OnAttack(Target);
}

void ANetNPC::MulticastPlayHitPresentation_Implementation(const FNetNPCHitPresentation& HitPresentation)
{
	BP_OnHit(HitPresentation);
}

void ANetNPC::OnDeath()
{
	if (bDeathPresentationStarted)
	{
		return;
	}

	bDeathPresentationStarted = true;
	GetWorldTimerManager().ClearTimer(AttackWindupTimerHandle);
	bAttackInProgress = false;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if (HasAuthority())
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
			{
				BrainComponent->StopLogic(TEXT("NPC died"));
			}
		}

		if (ANetGameMode* NetGameMode = GetWorld()->GetAuthGameMode<ANetGameMode>())
		{
			NetGameMode->NotifyEnemyKilled(this, LastDamageInstigator.Get());
		}

		ForceNetUpdate();
		SetLifeSpan(DeathDestroyDelay);
	}

	BP_OnDeath();
}

FNetNPCHitPresentation ANetNPC::BuildHitPresentation(
	float AppliedDamage,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser) const
{
	FHitResult BestHit;
	FVector HitDirection = FVector::ZeroVector;
	DamageEvent.GetBestHitInfo(this, DamageCauser, BestHit, HitDirection);

	if (HitDirection.IsNearlyZero())
	{
		const APawn* InstigatorPawn = EventInstigator ? EventInstigator->GetPawn() : nullptr;
		const AActor* DirectionSource = InstigatorPawn ? static_cast<const AActor*>(InstigatorPawn) : DamageCauser;
		if (DirectionSource)
		{
			HitDirection = GetActorLocation() - DirectionSource->GetActorLocation();
		}
	}

	FNetNPCHitPresentation Result;
	Result.AppliedDamage = AppliedDamage;
	Result.HitLocation = BestHit.bBlockingHit ? BestHit.ImpactPoint : GetActorLocation();
	Result.HitDirection = HitDirection.GetSafeNormal();
	Result.bFatal = IsDead();
	return Result;
}

void ANetNPC::ApplyHitKnockback(const FVector& HitDirection)
{
	FVector HorizontalDirection(HitDirection.X, HitDirection.Y, 0.0f);
	HorizontalDirection = HorizontalDirection.GetSafeNormal();
	if (HorizontalDirection.IsNearlyZero() || !GetCharacterMovement())
	{
		return;
	}

	const FVector KnockbackImpulse =
		(HorizontalDirection * HitKnockbackStrength)
		+ (FVector::UpVector * HitKnockbackVerticalBoost);
	GetCharacterMovement()->AddImpulse(KnockbackImpulse, true);
}
