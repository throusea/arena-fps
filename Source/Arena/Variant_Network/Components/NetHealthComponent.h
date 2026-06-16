// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NetHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetHealthChangedSignature, float, Health);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNetDeathSignature);

/**
 * Replicated health state for network gameplay actors.
 */
UCLASS(ClassGroup=(Network), meta=(BlueprintSpawnableComponent))
class ARENA_API UNetHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNetHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Applies damage on the authoritative instance. */
	UFUNCTION(BlueprintCallable, Category="Health")
	float ApplyHealthDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return bIsDead; }

	UPROPERTY(BlueprintAssignable, Category="Health")
	FNetHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FNetDeathSignature OnDeath;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_IsDead();

	void HandleDeath();

protected:
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Health", meta=(ClampMin=1))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_Health, BlueprintReadOnly, Category="Health")
	float Health = 100.0f;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_IsDead, BlueprintReadOnly, Category="Health")
	bool bIsDead = false;
};
