// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ArenaCameraManager.generated.h"

/**
 *  Basic First Person camera manager.
 *  Limits min/max look pitch.
 */
UCLASS()
class AArenaCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	/** Constructor */
	AArenaCameraManager();
};
