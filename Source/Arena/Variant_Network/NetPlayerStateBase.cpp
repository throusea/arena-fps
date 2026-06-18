// Fill out your copyright notice in the Description page of Project Settings.


#include "NetPlayerStateBase.h"
#include "Net/UnrealNetwork.h"

ANetPlayerStateBase::ANetPlayerStateBase()
{
	// Asc = CreateDefaultSubobject<UAbilitySystemComponent>(FName("Asc"));
	//
	// Asc->SetIsReplicated(true);
	// Asc->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void ANetPlayerStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetPlayerStateBase, KillScore);
}

void ANetPlayerStateBase::AddKillScore(int32 ScoreDelta)
{
	if (!HasAuthority() || ScoreDelta <= 0)
	{
		return;
	}

	KillScore += ScoreDelta;
	SetScore(static_cast<float>(KillScore));
	OnKillScoreChanged.Broadcast(KillScore);
}

void ANetPlayerStateBase::OnRep_KillScore()
{
	OnKillScoreChanged.Broadcast(KillScore);
}

// UAbilitySystemComponent* ANetPlayerStateBase::GetAbilitySystemComponent() const
// {
// 	return Asc;
// }
