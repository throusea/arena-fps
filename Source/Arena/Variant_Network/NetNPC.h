// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NetNPC.generated.h"

class UNetHealthComponent;
class AController;

/**
 * Minimal network NPC that can receive server-authoritative damage.
 */
UCLASS(Blueprintable)
class ARENA_API ANetNPC : public ACharacter
{
	GENERATED_BODY()

public:
	ANetNPC();

	/** Applies incoming engine damage to the replicated health component. */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Returns the replicated health component. */
	UNetHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/** Returns true once this NPC has died. */
	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const;

	/** Returns the attack range used by the AI controller. */
	UFUNCTION(BlueprintPure, Category="Attack")
	float GetAttackRange() const { return AttackRange; }

	/** Attempts a server-authoritative melee attack against the target. */
	UFUNCTION(BlueprintCallable, Category="Attack")
	bool TryAttack(AActor* Target);

protected:
	virtual void BeginPlay() override;

	/** Called when replicated health reaches zero. */
	UFUNCTION()
	void OnDeath();

private:
	/** Replicated health state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNetHealthComponent> HealthComponent;

	/** Distance at which this NPC can attack a player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Attack", meta=(AllowPrivateAccess="true", ClampMin=0, Units="cm"))
	float AttackRange = 180.0f;

	/** Damage applied by each NPC attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Attack", meta=(AllowPrivateAccess="true", ClampMin=0))
	float AttackDamage = 10.0f;

	/** Minimum time between attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Attack", meta=(AllowPrivateAccess="true", ClampMin=0, Units="s"))
	float AttackCooldown = 1.0f;

	TWeakObjectPtr<AController> LastDamageInstigator;

	float LastAttackTime = -1000.0f;
};
