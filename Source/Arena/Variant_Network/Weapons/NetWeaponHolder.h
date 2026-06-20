// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NetWeaponHolder.generated.h"

class ANetRifle;
class UAnimMontage;

UINTERFACE(MinimalAPI)
class UNetWeaponHolder : public UInterface
{
	GENERATED_BODY()
};

/** Common interface used by network weapons to communicate with their holder. */
class ARENA_API INetWeaponHolder
{
	GENERATED_BODY()

public:
	virtual void AttachWeaponMeshes(ANetRifle* Weapon) = 0;
	virtual void PlayFiringMontage(UAnimMontage* Montage) = 0;
	virtual void AddWeaponRecoil(float Recoil) = 0;
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) = 0;
	virtual FVector GetWeaponTargetLocation() = 0;
	virtual void AddWeaponClass(const TSubclassOf<ANetRifle>& WeaponClass) = 0;
	virtual void OnWeaponActivated(ANetRifle* Weapon) = 0;
	virtual void OnWeaponDeactivated(ANetRifle* Weapon) = 0;
	virtual void OnSemiWeaponRefire() = 0;
};
