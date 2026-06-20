// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NetWeaponHolder.generated.h"

class ANetWeaponBase;
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
	virtual void AttachWeaponMeshes(ANetWeaponBase* Weapon) = 0;
	virtual void PlayFiringMontage(UAnimMontage* Montage) = 0;
	virtual void AddWeaponRecoil(float Recoil) = 0;
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) = 0;
	virtual FVector GetWeaponTargetLocation() = 0;
	virtual void AddWeaponClass(const TSubclassOf<ANetWeaponBase>& WeaponClass) = 0;
	virtual void OnWeaponActivated(ANetWeaponBase* Weapon) = 0;
	virtual void OnWeaponDeactivated(ANetWeaponBase* Weapon) = 0;
	virtual void OnSemiWeaponRefire() = 0;
};
