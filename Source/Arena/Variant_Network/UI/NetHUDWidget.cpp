// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Network/UI/NetHUDWidget.h"
#include "Variant_Network/Components/NetHealthComponent.h"
#include "Variant_Network/NetCharacter.h"
#include "Variant_Network/NetGameState.h"
#include "Variant_Network/NetPlayerStateBase.h"
#include "Variant_Network/Weapons/NetRifle.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"

void UNetHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindToDataSources();
	RefreshHUD();
}

void UNetHUDWidget::NativeDestruct()
{
	UnbindFromDataSources();

	Super::NativeDestruct();
}

void UNetHUDWidget::RefreshHUD()
{
	BindToDataSources();
	RefreshHealth();
	RefreshScore();
	RefreshVictoryState();
	RefreshAmmo();
	RefreshWeaponName();
	RefreshSpread();
}

void UNetHUDWidget::HandleHealthChanged(float Health)
{
	const UNetHealthComponent* HealthComponent = ObservedHealthComponent.Get();
	BP_OnHealthChanged(Health, HealthComponent ? HealthComponent->GetMaxHealth() : 0.0f);
}

void UNetHUDWidget::HandleScoreChanged(int32 NewScore)
{
	BP_OnScoreChanged(NewScore);
}

void UNetHUDWidget::HandleVictoryStateChanged()
{
	RefreshVictoryState();
}

void UNetHUDWidget::HandleCurrentRifleChanged(ANetRifle* CurrentRifle)
{
	BP_OnCurrentRifleChanged(CurrentRifle);

	BindToRifle(CurrentRifle);
	RefreshAmmo();
	RefreshWeaponName();
	RefreshSpread();
}

void UNetHUDWidget::HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize)
{
	BP_OnAmmoChanged(CurrentAmmo, MagazineSize);
}

void UNetHUDWidget::HandleWeaponFired(const FNetWeaponShotResult& ShotResult)
{
	BP_OnCrosshairFired(ShotResult);
}

void UNetHUDWidget::HandleWeaponHit(const FNetWeaponShotResult& ShotResult)
{
	if (ShotResult.bDamagedActor)
	{
		BP_OnHitConfirmed(ShotResult);
	}
}

void UNetHUDWidget::HandleSpreadChanged(float SpreadAngle, float NormalizedSpread)
{
	BP_OnCrosshairSpreadChanged(SpreadAngle, NormalizedSpread);
}

void UNetHUDWidget::BP_OnCrosshairSpreadChanged_Implementation(float SpreadAngle, float NormalizedSpread)
{
	if (CrossHair)
	{
		const float CrosshairScale = FMath::Lerp(
			MinCrosshairScale,
			MaxCrosshairScale,
			FMath::Clamp(NormalizedSpread, 0.0f, 1.0f));
		CrossHair->SetRenderScale(FVector2D(CrosshairScale));
	}
}

void UNetHUDWidget::BindToDataSources()
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	BindToCharacter(Cast<ANetCharacter>(OwningPlayer->GetPawn()));
	BindToPlayerState(OwningPlayer->GetPlayerState<ANetPlayerStateBase>());
	BindToGameState(GetWorld() ? GetWorld()->GetGameState<ANetGameState>() : nullptr);
}

void UNetHUDWidget::UnbindFromDataSources()
{
	if (UNetHealthComponent* HealthComponent = ObservedHealthComponent.Get())
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &UNetHUDWidget::HandleHealthChanged);
	}

	if (ANetCharacter* Character = ObservedCharacter.Get())
	{
		Character->OnCurrentRifleChanged.RemoveDynamic(this, &UNetHUDWidget::HandleCurrentRifleChanged);
	}

	if (ANetRifle* Rifle = ObservedRifle.Get())
	{
		Rifle->OnAmmoChanged.RemoveDynamic(this, &UNetHUDWidget::HandleAmmoChanged);
		Rifle->OnWeaponFired.RemoveDynamic(this, &UNetHUDWidget::HandleWeaponFired);
		Rifle->OnWeaponHit.RemoveDynamic(this, &UNetHUDWidget::HandleWeaponHit);
		Rifle->OnSpreadChanged.RemoveDynamic(this, &UNetHUDWidget::HandleSpreadChanged);
	}

	if (ANetPlayerStateBase* PlayerState = ObservedPlayerState.Get())
	{
		PlayerState->OnKillScoreChanged.RemoveDynamic(this, &UNetHUDWidget::HandleScoreChanged);
	}

	if (ANetGameState* GameState = ObservedGameState.Get())
	{
		GameState->OnVictoryStateChanged.RemoveDynamic(this, &UNetHUDWidget::HandleVictoryStateChanged);
	}

	ObservedCharacter.Reset();
	ObservedHealthComponent.Reset();
	ObservedRifle.Reset();
	ObservedPlayerState.Reset();
	ObservedGameState.Reset();
}

void UNetHUDWidget::BindToCharacter(ANetCharacter* NewCharacter)
{
	if (ObservedCharacter.Get() == NewCharacter)
	{
		return;
	}

	if (UNetHealthComponent* OldHealthComponent = ObservedHealthComponent.Get())
	{
		OldHealthComponent->OnHealthChanged.RemoveDynamic(this, &UNetHUDWidget::HandleHealthChanged);
	}

	if (ANetCharacter* OldCharacter = ObservedCharacter.Get())
	{
		OldCharacter->OnCurrentRifleChanged.RemoveDynamic(this, &UNetHUDWidget::HandleCurrentRifleChanged);
	}

	ObservedCharacter = NewCharacter;
	ObservedHealthComponent.Reset();
	BindToRifle(nullptr);

	if (!NewCharacter)
	{
		return;
	}

	UNetHealthComponent* HealthComponent = NewCharacter->GetHealthComponent();
	if (!HealthComponent)
	{
		return;
	}

	ObservedHealthComponent = HealthComponent;
	HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &UNetHUDWidget::HandleHealthChanged);
	NewCharacter->OnCurrentRifleChanged.AddUniqueDynamic(this, &UNetHUDWidget::HandleCurrentRifleChanged);
	BindToRifle(NewCharacter->GetCurrentRifle());
}

void UNetHUDWidget::BindToRifle(ANetRifle* NewRifle)
{
	if (ObservedRifle.Get() == NewRifle)
	{
		return;
	}

	if (ANetRifle* OldRifle = ObservedRifle.Get())
	{
		OldRifle->OnAmmoChanged.RemoveDynamic(this, &UNetHUDWidget::HandleAmmoChanged);
		OldRifle->OnWeaponFired.RemoveDynamic(this, &UNetHUDWidget::HandleWeaponFired);
		OldRifle->OnWeaponHit.RemoveDynamic(this, &UNetHUDWidget::HandleWeaponHit);
		OldRifle->OnSpreadChanged.RemoveDynamic(this, &UNetHUDWidget::HandleSpreadChanged);
	}

	ObservedRifle = NewRifle;

	if (NewRifle)
	{
		NewRifle->OnAmmoChanged.AddUniqueDynamic(this, &UNetHUDWidget::HandleAmmoChanged);
		NewRifle->OnWeaponFired.AddUniqueDynamic(this, &UNetHUDWidget::HandleWeaponFired);
		NewRifle->OnWeaponHit.AddUniqueDynamic(this, &UNetHUDWidget::HandleWeaponHit);
		NewRifle->OnSpreadChanged.AddUniqueDynamic(this, &UNetHUDWidget::HandleSpreadChanged);
	}
}

void UNetHUDWidget::BindToPlayerState(ANetPlayerStateBase* NewPlayerState)
{
	if (ObservedPlayerState.Get() == NewPlayerState)
	{
		return;
	}

	if (ANetPlayerStateBase* OldPlayerState = ObservedPlayerState.Get())
	{
		OldPlayerState->OnKillScoreChanged.RemoveDynamic(this, &UNetHUDWidget::HandleScoreChanged);
	}

	ObservedPlayerState = NewPlayerState;

	if (NewPlayerState)
	{
		NewPlayerState->OnKillScoreChanged.AddUniqueDynamic(this, &UNetHUDWidget::HandleScoreChanged);
	}
}

void UNetHUDWidget::BindToGameState(ANetGameState* NewGameState)
{
	if (ObservedGameState.Get() == NewGameState)
	{
		return;
	}

	if (ANetGameState* OldGameState = ObservedGameState.Get())
	{
		OldGameState->OnVictoryStateChanged.RemoveDynamic(this, &UNetHUDWidget::HandleVictoryStateChanged);
	}

	ObservedGameState = NewGameState;

	if (NewGameState)
	{
		NewGameState->OnVictoryStateChanged.AddUniqueDynamic(this, &UNetHUDWidget::HandleVictoryStateChanged);
	}
}

void UNetHUDWidget::RefreshHealth()
{
	const UNetHealthComponent* HealthComponent = ObservedHealthComponent.Get();
	BP_OnHealthChanged(
		HealthComponent ? HealthComponent->GetHealth() : 0.0f,
		HealthComponent ? HealthComponent->GetMaxHealth() : 0.0f
	);
}

void UNetHUDWidget::RefreshScore()
{
	const ANetPlayerStateBase* PlayerState = ObservedPlayerState.Get();
	BP_OnScoreChanged(PlayerState ? PlayerState->GetKillScore() : 0);
}

void UNetHUDWidget::RefreshVictoryState()
{
	const ANetGameState* GameState = ObservedGameState.Get();
	BP_OnVictoryStateChanged(
		GameState && GameState->HasWinner(),
		GameState ? GameState->GetWinnerPlayerState() : nullptr
	);
}

void UNetHUDWidget::RefreshAmmo()
{
	const ANetRifle* Rifle = ObservedRifle.Get();
	BP_OnAmmoChanged(
		Rifle ? Rifle->GetCurrentAmmo() : 0,
		Rifle ? Rifle->GetMagazineSize() : 0
	);
}

void UNetHUDWidget::RefreshWeaponName()
{
	const ANetRifle* Rifle = ObservedRifle.Get();
	BP_OnWeaponNameChanged(Rifle ? Rifle->GetWeaponName() : FText::GetEmpty());
}

void UNetHUDWidget::RefreshSpread()
{
	const ANetRifle* Rifle = ObservedRifle.Get();
	BP_OnCrosshairSpreadChanged(
		Rifle ? Rifle->GetCurrentSpreadAngle() : 0.0f,
		Rifle ? Rifle->GetNormalizedSpread() : 0.0f);
}
