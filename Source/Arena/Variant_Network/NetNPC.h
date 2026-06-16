// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NetNPC.generated.h"

class UNetHealthComponent;

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

protected:
	virtual void BeginPlay() override;

	/** Called when replicated health reaches zero. */
	UFUNCTION()
	void OnDeath();

private:
	/** Replicated health state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNetHealthComponent> HealthComponent;
};
