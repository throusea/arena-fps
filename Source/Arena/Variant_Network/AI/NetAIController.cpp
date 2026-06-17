// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetAIController.h"
#include "Variant_Network/NetCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Engine/World.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"

ANetAIController::ANetAIController()
{
	bStartAILogicOnPossess = true;
	bAttachToPawn = true;

	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 3000.0f;
	SightConfig->LoseSightRadius = 3500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 75.0f;
	SightConfig->SetMaxAge(3.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
}

void ANetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HasAuthority())
	{
		AIPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ANetAIController::OnTargetPerceptionUpdated);

		if (BehaviorTreeAsset)
		{
			RunBehaviorTree(BehaviorTreeAsset);
		}
	}
}

void ANetAIController::OnUnPossess()
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ANetAIController::OnTargetPerceptionUpdated);
	}

	KnownTargets.Reset();
	CurrentTarget.Reset();
	ClearFocus(EAIFocusPriority::Gameplay);

	Super::OnUnPossess();
}

void ANetAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ANetAIController::OnTargetPerceptionUpdated);
	}

	Super::EndPlay(EndPlayReason);
}

void ANetAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ANetCharacter* Target = Cast<ANetCharacter>(Actor);
	if (!Target)
	{
		return;
	}

	// UE_LOG(LogTemp, Error, TEXT("%s"), *GetNameSafe(Target));

	if (Stimulus.WasSuccessfullySensed() && IsValidTarget(Target))
	{
		KnownTargets.AddUnique(Target);
	}
	else if (!IsActorCurrentlySensed(Target))
	{
		KnownTargets.RemoveAll(
			[Target](const TWeakObjectPtr<ANetCharacter>& TargetPtr)
			{
				return TargetPtr.Get() == Target;
			}
		);
	}

	RefreshTargetSelection();
}

void ANetAIController::RefreshTargetSelection()
{
	RemoveInvalidKnownTargets();

	ANetCharacter* ExistingTarget = CurrentTarget.Get();
	if (IsValidTarget(ExistingTarget) && IsActorCurrentlySensed(ExistingTarget))
	{
		SetCurrentTarget(ExistingTarget);
		return;
	}

	SetCurrentTarget(FindClosestKnownTarget());
}

void ANetAIController::SetCurrentTarget(ANetCharacter* NewTarget)
{
	CurrentTarget = NewTarget;

	// UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (NewTarget)
	{
		SetFocus(NewTarget);

		if (Blackboard)
		{
			Blackboard->SetValueAsObject(TargetActorKeyName, NewTarget);
			Blackboard->SetValueAsVector(TargetLocationKeyName, NewTarget->GetActorLocation());
			Blackboard->SetValueAsBool(HasLineOfSightKeyName, true);
		}
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);

		if (Blackboard)
		{
			Blackboard->ClearValue(TargetActorKeyName);
			Blackboard->ClearValue(TargetLocationKeyName);
			Blackboard->SetValueAsBool(HasLineOfSightKeyName, false);
		}
	}
}

ANetCharacter* ANetAIController::FindClosestKnownTarget() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return nullptr;
	}

	ANetCharacter* ClosestTarget = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<ANetCharacter>& TargetPtr : KnownTargets)
	{
		ANetCharacter* Target = TargetPtr.Get();
		if (!IsValidTarget(Target))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), Target->GetActorLocation());
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestTarget = Target;
		}
	}

	return ClosestTarget;
}

bool ANetAIController::IsValidTarget(const ANetCharacter* Target) const
{
	return IsValid(Target) && !Target->IsDead();
}

bool ANetAIController::IsActorCurrentlySensed(AActor* Actor) const
{
	if (!AIPerception || !Actor)
	{
		return false;
	}

	FActorPerceptionBlueprintInfo PerceptionInfo;
	AIPerception->GetActorsPerception(Actor, PerceptionInfo);

	for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			return true;
		}
	}

	return false;
}

void ANetAIController::RemoveInvalidKnownTargets()
{
	KnownTargets.RemoveAll(
		[this](const TWeakObjectPtr<ANetCharacter>& TargetPtr)
		{
			ANetCharacter* Target = TargetPtr.Get();
			return !IsValidTarget(Target) || !IsActorCurrentlySensed(Target);
		}
	);
}
