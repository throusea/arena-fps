// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "NetAIController.generated.h"

class ANetCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;

/**
 * Server-authoritative AI controller that maintains perceived player targets for Behavior Trees.
 */
UCLASS()
class ARENA_API ANetAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANetAIController();

	/** Returns the currently selected target actor. */
	UFUNCTION(BlueprintPure, Category="AI|Targeting")
	ANetCharacter* GetCurrentTarget() const { return CurrentTarget.Get(); }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void RefreshTargetSelection();
	void SetCurrentTarget(ANetCharacter* NewTarget);
	ANetCharacter* FindClosestKnownTarget() const;
	bool IsValidTarget(const ANetCharacter* Target) const;
	bool IsActorCurrentlySensed(AActor* Actor) const;
	void RemoveInvalidKnownTargets();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Optional behavior tree to run when this controller possesses an NPC. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Behavior", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/** Blackboard object key that stores the selected target actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Blackboard", meta=(AllowPrivateAccess="true"))
	FName TargetActorKeyName = TEXT("TargetActor");

	/** Blackboard vector key that stores the selected target's last known location. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Blackboard", meta=(AllowPrivateAccess="true"))
	FName TargetLocationKeyName = TEXT("TargetLocation");

	/** Blackboard bool key that stores whether this controller has the sight of target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Blackboard", meta=(AllowPrivateAccess="true"))
	FName HasLineOfSightKeyName = TEXT("HasLineOfSight");

	TArray<TWeakObjectPtr<ANetCharacter>> KnownTargets;
	TWeakObjectPtr<ANetCharacter> CurrentTarget;
};
