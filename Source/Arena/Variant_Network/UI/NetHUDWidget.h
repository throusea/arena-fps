// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Variant_Network/NetGameState.h"
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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Health Changed"))
	void BP_OnHealthChanged(float Health, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Score Changed"))
	void BP_OnScoreChanged(int32 Score);

	/** Updates the owning player's replicated display name. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Player Name Changed"))
	void BP_OnPlayerNameChanged(const FString& PlayerName);

	/** Rebuilds the multiplayer scoreboard from the sorted GameState player list. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD|Scoreboard", meta=(DisplayName="Scoreboard Changed"))
	void BP_OnScoreboardChanged(const TArray<FNetScoreboardEntry>& Entries);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Victory State Changed"))
	void BP_OnVictoryStateChanged(
		bool bHasWinner,
		ANetPlayerStateBase* WinnerPlayerState,
		const FString& WinnerPlayerName);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Ammo Changed"))
	void BP_OnAmmoChanged(int32 CurrentAmmo, int32 MagazineSize);

	/** Updates reload visibility, progress bar percent and optional countdown text. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD|Weapon", meta=(DisplayName="Reload Progress Changed"))
	void BP_OnReloadProgressChanged(bool bIsReloading, float ReloadProgress, float RemainingTime);

	UFUNCTION(BlueprintImplementableEvent, Category="HUD", meta=(DisplayName="Rifle Changed"))
	void BP_OnCurrentRifleChanged(ANetWeaponBase* CurrentRifle);

	/** Starts the crosshair firing feedback animation. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD|Weapon", meta=(DisplayName="Crosshair Fired"))
	void BP_OnCrosshairFired(const FNetWeaponFireResult& FireResult);

	/** Shows hit feedback from a server-confirmed weapon hit. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD|Weapon", meta=(DisplayName="Hit Confirmed"))
	void BP_OnHitConfirmed(const FNetWeaponImpactResult& ImpactResult);

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

	UFUNCTION()
	void HandlePlayerNameChanged(const FString& NewPlayerName);

	/** Bind Function for Victory State Changed Event */
	UFUNCTION()
	void HandleVictoryStateChanged();

	UFUNCTION()
	void HandleScoreboardChanged();

	/** Bind Function for Handle Current Rifle Changed Event */
	UFUNCTION()
	void HandleCurrentRifleChanged(ANetWeaponBase* CurrentRifle);

	/** Bind Function for Handle Ammo Changed Event */
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize);

	UFUNCTION()
	void HandleReloadStateChanged(bool bIsReloading);

	UFUNCTION()
	void HandleWeaponFired(const FNetWeaponFireResult& FireResult);

	UFUNCTION()
	void HandleWeaponHit(const FNetWeaponImpactResult& ImpactResult);

	UFUNCTION()
	void HandleSpreadChanged(float SpreadAngle, float NormalizedSpread);

	/** Bind All Events to Data Sources */
	void BindToDataSources();

	/** Unbind All Events to Data Sources */
	void UnbindFromDataSources();
	void BindToCharacter(ANetCharacter* NewCharacter);
	void BindToRifle(ANetWeaponBase* NewRifle);
	void BindToPlayerState(ANetPlayerStateBase* NewPlayerState);
	void BindToGameState(ANetGameState* NewGameState);

	void RefreshHealth();
	void RefreshScore();
	void RefreshPlayerName();
	void RefreshScoreboard();
	void RefreshVictoryState();
	void RefreshAmmo();
	void RefreshReload();
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
	TWeakObjectPtr<ANetWeaponBase> ObservedRifle;

	UPROPERTY(Transient)
	TWeakObjectPtr<ANetPlayerStateBase> ObservedPlayerState;

	UPROPERTY(Transient)
	TWeakObjectPtr<ANetGameState> ObservedGameState;
};
