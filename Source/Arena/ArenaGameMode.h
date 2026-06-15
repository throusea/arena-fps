// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();
};
