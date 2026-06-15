// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetPlayerStateBase.generated.h"

/**
 *
 */
UCLASS()
class ARENA_API ANetPlayerStateBase : public APlayerState
{
	GENERATED_BODY()

public:
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	// UAbilitySystemComponent* Asc;

public:

	ANetPlayerStateBase();

public:

	// virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
};
