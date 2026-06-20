// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Weapons/NetWeaponHolder.h"
#include "NetCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class ANetRifle;
class ANetWeaponBase;
class UNetHealthComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetCurrentRifleChangedSignature, ANetWeaponBase*, CurrentRifle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNetWeaponAmmoUpdatedSignature, int32, CurrentAmmo, int32, MagazineSize);

// DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ANetCharacter : public ACharacter, public INetWeaponHolder
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Local camera activated when this character dies. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UCameraComponent* DeathCamera;

	/** Replicated health state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNetHealthComponent> HealthComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Fire rifle input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Rifle class spawned for this character */
	UPROPERTY(EditAnywhere, Category="Weapon")
	TSubclassOf<ANetWeaponBase> RifleClass;

	/** First-person mesh socket used for equipped weapons. */
	UPROPERTY(EditAnywhere, Category="Weapon")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Third-person mesh socket used for equipped weapons. */
	UPROPERTY(EditAnywhere, Category="Weapon")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Local correction applied after the third-person weapon snaps to its socket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attachment")
	FTransform ThirdPersonWeaponAttachmentOffset = FTransform(FRotator(-10.0f, 0.0f, 0.0f));

	/** Maximum distance used when calculating the weapon target. */
	UPROPERTY(EditAnywhere, Category="Weapon|Aim", meta=(ClampMin=0, Units="cm"))
	float MaxAimDistance = 10000.0f;

	/** Rifle currently owned by this character */
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_CurrentRifle, Category="Weapon")
	TObjectPtr<ANetWeaponBase> CurrentRifle;

public:
	ANetCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Called when replicated health reaches zero. */
	UFUNCTION()
	void OnDeath();

	/** Allows Blueprint to play local character death presentation. */
	UFUNCTION(BlueprintImplementableEvent, Category="Character|Presentation", meta=(DisplayName="On Death"))
	void BP_OnDeath();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Registers replicated properties. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Gameplay cleanup */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Server Only */
	virtual void PossessedBy(AController* NewController) override;

	/** Client Only */
	virtual void OnRep_Controller() override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Starts firing the equipped rifle */
	void DoStartFiring();

	/** Stops firing the equipped rifle */
	void DoStopFiring();

public:
	//~Begin INetWeaponHolder interface
	virtual void AttachWeaponMeshes(ANetWeaponBase* Weapon) override;
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;
	virtual void AddWeaponRecoil(float Recoil) override;
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;
	virtual FVector GetWeaponTargetLocation() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	virtual void AddWeaponClass(const TSubclassOf<ANetWeaponBase>& WeaponClass) override;

	virtual void OnWeaponActivated(ANetWeaponBase* Weapon) override;
	virtual void OnWeaponDeactivated(ANetWeaponBase* Weapon) override;
	virtual void OnSemiWeaponRefire() override;
	//~End INetWeaponHolder interface

	/** Server-side start firing request from owning client. */
	UFUNCTION(Server, Reliable)
	void ServerStartFiring();

	void HandleStartFiring();

	/** Server-side stop firing request from owning client. */
	UFUNCTION(Server, Reliable)
	void ServerStopFiring();

	void HandleStopFiring();

	/** Applies incoming engine damage to the health component. */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Returns the camera used for local death presentation. */
	UCameraComponent* GetDeathCamera() const { return DeathCamera; }

	/** Returns the replicated health component. */
	UNetHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	ANetWeaponBase* GetCurrentRifle() const { return CurrentRifle; }

	/** Returns true once this character has died. */
	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FNetCurrentRifleChangedSignature OnCurrentRifleChanged;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Ammo")
	FNetWeaponAmmoUpdatedSignature OnWeaponAmmoUpdated;

private:
	UFUNCTION()
	void OnRep_CurrentRifle(ANetWeaponBase* PreviousRifle);

	void ApplyCurrentRifle(ANetWeaponBase* PreviousRifle);

};
