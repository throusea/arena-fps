// Copyright Epic Games, Inc. All Rights Reserved.


#include "NetPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "ArenaCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Arena.h"
#include "UI/NetHUDWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

ANetPlayerController::ANetPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AArenaCameraManager::StaticClass();
}

void ANetPlayerController::BeginPlay()
{
	Super::BeginPlay();


	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogArena, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	if (IsLocalPlayerController())
	{
		CreateHUDWidget();
	}
}

void ANetPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RefreshHUDWidget();
}

void ANetPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	RefreshHUDWidget();
}

void ANetPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RefreshHUDWidget();
}

void ANetPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}

}

bool ANetPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ANetPlayerController::CreateHUDWidget()
{
	if (HUDWidget || !HUDWidgetClass)
	{
		return;
	}

	HUDWidget = CreateWidget<UNetHUDWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToPlayerScreen(0);
		HUDWidget->RefreshHUD();
	}
	else
	{
		UE_LOG(LogArena, Error, TEXT("Could not spawn network HUD widget."));
	}
}

void ANetPlayerController::RefreshHUDWidget() const
{
	if (HUDWidget)
	{
		HUDWidget->RefreshHUD();
	}
}
