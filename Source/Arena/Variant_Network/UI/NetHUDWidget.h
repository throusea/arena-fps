// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Network/Weapons/NetRifle.h"
#include "NetHUDWidget.generated.h"

class ANetCharacter;
class ANetGameState;
class ANetPlayerStateBase;
class UNetHealthComponent;
class UWidget;

/**
 * C++ binding layer for the network HUD. Blueprint implements the visual updates.
 */
UCLASS(Abstract, Blueprintable)
class ARENA_API UNetHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="HUD")
	void RefreshHUD();

	UFUNCTION(BlueprintPure, Category="HUD")
	ANetCharacter* GetObservedCharacter() const { return ObservedCharacter.Get(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Health Changed"))
	void BP_OnHealthChanged(float Health, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Score Changed"))
	void BP_OnScoreChanged(int32 Score);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Victory State Changed"))
	void BP_OnVictoryStateChanged(bool bHasWinner, ANetPlayerStateBase* WinnerPlayerState);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Ammo Changed"))
	void BP_OnAmmoChanged(int32 CurrentAmmo, int32 MagazineSize);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Rifle Changed"))
	void BP_OnCurrentRifleChanged(ANetRifle* CurrentRifle);

	/** Starts the crosshair firing feedback animation. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD|Weapon", meta=(DisplayName="Crosshair Fired"))
	void BP_OnCrosshairFired(const FNetWeaponShotResult& ShotResult);

	/** Shows hit feedback from a server-confirmed weapon hit. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD|Weapon", meta=(DisplayName="Hit Confirmed"))
	void BP_OnHitConfirmed(const FNetWeaponShotResult& ShotResult);

	/** Updates the displayed name of the equipped weapon. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD|Weapon", meta=(DisplayName="Weapon Name Changed"))
	void BP_OnWeaponNameChanged(const FText& WeaponName);

	/** Updates crosshair size from the weapon's authoritative spread angle. */
	UFUNCTION(BlueprintNativeEvent, Category="HUD|Weapon", meta=(DisplayName="Crosshair Spread Changed"))
	void BP_OnCrosshairSpreadChanged(float SpreadAngle, float NormalizedSpread);
	virtual void BP_OnCrosshairSpreadChanged_Implementation(float SpreadAngle, float NormalizedSpread);

private:
	/** Bind Function for Health Changed Event */
	UFUNCTION()
	void HandleHealthChanged(float Health);

	/** Bind Function for Score Changed Event */
	UFUNCTION()
	void HandleScoreChanged(int32 NewScore);

	/** Bind Function for Victory State Changed Event */
	UFUNCTION()
	void HandleVictoryStateChanged();

	/** Bind Function for Handle Current Rifle Changed Event */
	UFUNCTION()
	void HandleCurrentRifleChanged(ANetRifle* CurrentRifle);

	/** Bind Function for Handle Ammo Changed Event */
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize);

	UFUNCTION()
	void HandleWeaponFired(const FNetWeaponShotResult& ShotResult);

	UFUNCTION()
	void HandleWeaponHit(const FNetWeaponShotResult& ShotResult);

	UFUNCTION()
	void HandleSpreadChanged(float SpreadAngle, float NormalizedSpread);

	/** Bind All Events to Data Sources */
	void BindToDataSources();

	/** Unbind All Events to Data Sources */
	void UnbindFromDataSources();
	void BindToCharacter(ANetCharacter* NewCharacter);
	void BindToRifle(ANetRifle* NewRifle);
	void BindToPlayerState(ANetPlayerStateBase* NewPlayerState);
	void BindToGameState(ANetGameState* NewGameState);

	void RefreshHealth();
	void RefreshScore();
	void RefreshVictoryState();
	void RefreshAmmo();
	void RefreshWeaponName();
	void RefreshSpread();

private:
	/** Optional crosshair root scaled by the default spread presentation. */
	UPROPERTY(BlueprintReadOnly, Category="HUD|Weapon", meta=(BindWidgetOptional, AllowPrivateAccess="true"))
	TObjectPtr<UWidget> CrossHair;

	UPROPERTY(EditDefaultsOnly, Category="HUD|Weapon", meta=(ClampMin=0))
	float MinCrosshairScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="HUD|Weapon", meta=(ClampMin=0))
	float MaxCrosshairScale = 2.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<ANetCharacter> ObservedCharacter;

	UPROPERTY(Transient)
	TWeakObjectPtr<UNetHealthComponent> ObservedHealthComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<ANetRifle> ObservedRifle;

	UPROPERTY(Transient)
	TWeakObjectPtr<ANetPlayerStateBase> ObservedPlayerState;

	UPROPERTY(Transient)
	TWeakObjectPtr<ANetGameState> ObservedGameState;
};
