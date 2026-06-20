// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NetNPC.generated.h"

class UNetHealthComponent;
class AController;

/** Server-authored data used by Blueprint hit presentation. */
USTRUCT(BlueprintType)
struct ARENA_API FNetNPCHitPresentation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Hit")
	float AppliedDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Hit")
	FVector_NetQuantize HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Hit")
	FVector_NetQuantizeNormal HitDirection = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Hit")
	bool bFatal = false;
};

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

	UFUNCTION()
	void DoAttack(AActor* Target);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAttackPresentation(AActor* Target);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitPresentation(const FNetNPCHitPresentation& HitPresentation);

	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Presentation", meta=(DisplayName="On Attack"))
	void BP_OnAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Presentation", meta=(DisplayName="On Hit"))
	void BP_OnHit(const FNetNPCHitPresentation& HitPresentation);

	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Presentation", meta=(DisplayName="On Death"))
	void BP_OnDeath();

	/** Called when replicated health reaches zero. */
	UFUNCTION()
	void OnDeath();

	FNetNPCHitPresentation BuildHitPresentation(
		float AppliedDamage,
		const FDamageEvent& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) const;

	void ApplyHitKnockback(const FVector& HitDirection);

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

	/** Delay between accepting an attack and performing its authoritative hit check. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Attack", meta=(AllowPrivateAccess="true", ClampMin=0, Units="s"))
	float AttackWindup = 0.5f;

	/** Horizontal velocity impulse applied by the server for a non-fatal hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Hit", meta=(AllowPrivateAccess="true", ClampMin=0, Units="cm/s"))
	float HitKnockbackStrength = 150.0f;

	/** Small upward component added to the non-fatal hit impulse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Hit", meta=(AllowPrivateAccess="true", ClampMin=0, Units="cm/s"))
	float HitKnockbackVerticalBoost = 20.0f;

	/** Keeps the replicated actor alive while clients play its death presentation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(AllowPrivateAccess="true", ClampMin=0.1, Units="s"))
	float DeathDestroyDelay = 3.0f;

	TWeakObjectPtr<AController> LastDamageInstigator;

	float LastAttackTime = -1000.0f;

	FTimerHandle AttackWindupTimerHandle;

	bool bAttackInProgress = false;

	bool bDeathPresentationStarted = false;
};
