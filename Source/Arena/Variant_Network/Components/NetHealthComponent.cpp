// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetHealthComponent.h"
#include "Net/UnrealNetwork.h"

UNetHealthComponent::UNetHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNetHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		Health = MaxHealth;
		bIsDead = Health <= 0.0f;
	}
}

void UNetHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNetHealthComponent, MaxHealth);
	DOREPLIFETIME(UNetHealthComponent, Health);
	DOREPLIFETIME(UNetHealthComponent, bIsDead);
}

float UNetHealthComponent::ApplyHealthDamage(float DamageAmount)
{
	if (GetOwnerRole() != ROLE_Authority || bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
	const float AppliedDamage = OldHealth - Health;

	OnHealthChanged.Broadcast(Health);

	if (Health <= 0.0f)
	{
		HandleDeath();
	}

	return AppliedDamage;
}

void UNetHealthComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health);
}

void UNetHealthComponent::OnRep_IsDead()
{
	if (bIsDead)
	{
		OnDeath.Broadcast();
	}
}

void UNetHealthComponent::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	OnDeath.Broadcast();
}
