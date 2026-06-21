// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "NetWeaponPickup.generated.h"

class ANetWeaponBase;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Data used to configure a network weapon pickup. */
USTRUCT(BlueprintType)
struct FNetWeaponPickupTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Mesh displayed while the pickup is available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** Weapon granted by the server when a holder overlaps the pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
	TSubclassOf<ANetWeaponBase> WeaponToSpawn;
};

/** Server-authoritative weapon pickup with replicated availability. */
UCLASS(Abstract)
class ARENA_API ANetWeaponPickup : public AActor
{
	GENERATED_BODY()

public:
	ANetWeaponPickup();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Data table row containing the pickup mesh and weapon class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
	FDataTableRowHandle WeaponType;

	/** Seconds before the pickup becomes available again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup", meta=(ClampMin=0, ClampMax=120, Units="s"))
	float RespawnTime = 4.0f;

	/** Called on every peer when the server grants this pickup. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta=(DisplayName="On Collected"))
	void BP_OnCollected(AActor* CollectingActor);

	/** Called on every peer when the pickup becomes available again. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta=(DisplayName="On Respawn"))
	void BP_OnRespawn();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> SphereCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TSubclassOf<ANetWeaponBase> WeaponClass;

	UPROPERTY(ReplicatedUsing=OnRep_IsAvailable, VisibleInstanceOnly, Category="Pickup")
	bool bIsAvailable = true;

	FTimerHandle RespawnTimer;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_IsAvailable(bool bWasAvailable);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastOnCollected(AActor* CollectingActor);

	void RespawnPickup();
	void SetPickupAvailable(bool bNewAvailable);
	void ApplyPickupState(bool bWasAvailable);
	void LoadWeaponData();
};
