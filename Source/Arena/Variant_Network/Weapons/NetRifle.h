// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Arena.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetWeaponHolder.h"
#include "NetRifle.generated.h"

class UNiagaraSystem;
class APawn;
class UAnimInstance;
class UAnimMontage;
class UCameraShakeBase;

UENUM(BlueprintType)
enum class ENetWeaponFireType : uint8
{
	Hitscan,
	Projectile
};

/** Server-authored data describing one weapon shot or confirmed hit. */
USTRUCT(BlueprintType)
struct ARENA_API FNetWeaponShotResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	FVector_NetQuantize TraceStart = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	FVector_NetQuantize TraceEnd = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	FVector_NetQuantizeNormal ImpactNormal = FVector::ZeroVector;

	/** Resolved on the receiving machine; may be null when the actor is destroyed or not network-relevant. */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	TObjectPtr<AActor> HitActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	bool bBlockingHit = false;

	/** True when the server applied positive damage to the hit actor. */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	bool bDamagedActor = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNetAmmoChangedSignature, int32, CurrentAmmo, int32, MagazineSize);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetWeaponFiredSignature, const FNetWeaponShotResult&, ShotResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetWeaponHitSignature, const FNetWeaponShotResult&, ShotResult);

/**
 * Minimal hitscan rifle for the network gameplay variant.
 */
UCLASS(Blueprintable)
class ARENA_API ANetRifle : public AActor
{
	GENERATED_BODY()

public:
	ANetRifle();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Start firing the rifle on Server. */
	void StartFiringOnServer();

	/** Stop firing the rifle on Server. */
	void StopFiringOnServer();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayFireEffects(const FNetWeaponShotResult& ShotResult);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitEffects(const FNetWeaponShotResult& ShotResult);

	/** Reports a projectile hit that has already been confirmed on the server. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	void ReportProjectileHit(const FHitResult& HitResult, bool bDamagedActor);

	/** Returns the last line trace result produced by this rifle. */
	UFUNCTION(BlueprintPure, Category="Weapon")
	FHitResult GetLastHitResult() const { return LastHitResult; }

	/** Returns true if the last shot hit a blocking target. */
	UFUNCTION(BlueprintPure, Category="Weapon")
	bool HasValidHitResult() const { return LastHitResult.bBlockingHit; }

	UFUNCTION(BlueprintPure, Category="Weapon|Ammo")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon|Ammo")
	int32 GetMagazineSize() const { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	FText GetWeaponName() const { return WeaponName; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	ENetWeaponFireType GetFireType() const { return FireType; }

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }
	TSubclassOf<UAnimInstance> GetFirstPersonAnimInstanceClass() const { return FirstPersonAnimInstanceClass; }
	TSubclassOf<UAnimInstance> GetThirdPersonAnimInstanceClass() const { return ThirdPersonAnimInstanceClass; }

	UPROPERTY(BlueprintAssignable, Category="Weapon|Ammo")
	FNetAmmoChangedSignature OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Effects")
	FNetWeaponFiredSignature OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Effects")
	FNetWeaponHitSignature OnWeaponHit;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Fire one hitscan shot if cooldown allows it on Server */
	void FireOnServer();

	/** Called when the fire cooldown expires. */
	void FireCooldownExpired();

	/** Performs the camera-centered rifle trace. */
	bool TraceFromCamera(FHitResult& OutHitResult) const;

	/** Performs one authoritative shot on the server. */
	FNetWeaponShotResult FireAuthoritativeShot();

	/** Spawns a configured projectile on the server. */
	void FireProjectileOnServer(const FNetWeaponShotResult& ShotResult);

	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Effects", meta=(DisplayName="Weapon Fired"))
	void BP_OnWeaponFired(const FNetWeaponShotResult& ShotResult);

	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Effects", meta=(DisplayName="Weapon Hit"))
	void BP_OnWeaponHit(const FNetWeaponShotResult& ShotResult);

	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Ammo", meta=(DisplayName="Weapon Ammo Changed"))
	void BP_OnAmmoChanged(int32 NewCurrentAmmo, int32 NewMagazineSize);

protected:
	/** Player-facing weapon name displayed by the HUD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FText WeaponName = NSLOCTEXT("NetWeapon", "DefaultWeaponName", "Rifle");

	/** Determines whether the weapon traces instantly or spawns a projectile. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ENetWeaponFireType FireType = ENetWeaponFireType::Hitscan;

	/** Actor class spawned by projectile weapons. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(EditCondition="FireType == ENetWeaponFireType::Projectile", EditConditionHides))
	TSubclassOf<AActor> ProjectileClass;

	/** Maximum trace distance from the camera center. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin=0, Units="cm"))
	float TraceDistance = 10000.0f;

	/** Trace channel used by the rifle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_WeaponTrace;

	/** Damage applied to the actor hit by the rifle trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin=0))
	float Damage = 20.0f;

	/** Half-angle of the server-authoritative shot cone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin=0, ClampMax=90, Units="Degrees"))
	float SpreadHalfAngle = 0.0f;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Weapon|Ammo", meta=(ClampMin=1))
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Ammo")
	bool bConsumeAmmo = false;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_CurrentAmmo, BlueprintReadOnly, Category="Weapon|Ammo")
	int32 CurrentAmmo = 30;

	/** Lifetime for the server-authoritative debug trace line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Debug", meta=(ClampMin=0, Units="s"))
	float DebugTraceDuration = 0.05f;

	/** Last hit result generated by the rifle. */
	UPROPERTY(VisibleInstanceOnly, Replicated, BlueprintReadOnly, Category="Weapon")
	FHitResult LastHitResult;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	FName MuzzleSocketName;

	UPROPERTY(EditAnywhere, Category="Aim", meta=(ClampMin=0, ClampMax=1000, Units="cm"))
	float MuzzleOffset = 10.f;

	UPROPERTY(EditAnywhere, Category="Refire")
	bool bFullAuto = false;

	UPROPERTY(EditAnywhere, Category="Animation")
	TObjectPtr<UAnimMontage> FiringMontage;

	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	UPROPERTY(EditAnywhere, Category="Aim", meta=(ClampMin=0, ClampMax=100))
	float FiringRecoil = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Effects")
	TSubclassOf<UCameraShakeBase> FiringCameraShake;

	UPROPERTY(EditAnywhere, Category="Effects", meta=(ClampMin=0))
	float CameraShakeScale = 1.0f;

	/** Minimum time between shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Refire", meta=(ClampMin=0, Units="s"))
	float FireCooldown = 0.2f;

	/** First person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USkeletalMeshComponent* FirstPersonMesh;

	/** Third person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USkeletalMeshComponent* ThirdPersonMesh;

	UPROPERTY(EditDefaultsOnly, Category="Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, Category="Effects")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, Category="Effects")
	TObjectPtr<USoundBase> ImpactSound;

	/** Pawn that owns this rifle. */
	UPROPERTY()
	TObjectPtr<APawn> PawnOwner;

	/** Interface used to communicate with the current weapon holder. */
	INetWeaponHolder* WeaponOwner = nullptr;

	bool bCanFire = true;
	bool bWantsToFire = false;

	FTimerHandle FireCooldownTimer;

private:
	UFUNCTION()
	void OnRep_CurrentAmmo();

	void BroadcastAmmoChanged();
};
