// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Arena.h"
#include "Components/NetHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/NetRifle.h"

ANetCharacter::ANetCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WeaponTrace, ECR_Block);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	HealthComponent = CreateDefaultSubobject<UNetHealthComponent>(TEXT("Health Component"));

	RifleClass = ANetRifle::StaticClass();
}

void ANetCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ANetCharacter::OnDeath);
	}

	if (HasAuthority() && RifleClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentRifle = GetWorld()->SpawnActor<ANetRifle>(RifleClass, GetActorTransform(), SpawnParams);
		if (CurrentRifle)
		{
			const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);
			CurrentRifle->AttachToActor(this, AttachmentRule);
			OnCurrentRifleChanged.Broadcast(CurrentRifle);
		}
	}
}

void ANetCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(CurrentRifle))
	{
		CurrentRifle->Destroy();
		CurrentRifle = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ANetCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	FString LocalRoleStr = StaticEnum<ENetRole>()->GetNameStringByValue(GetLocalRole());
	FString RemoteRoleStr = StaticEnum<ENetRole>()->GetNameStringByValue(GetRemoteRole());

	const FString Msg = FString::Printf(
		TEXT("Actor=%s LocalRole=%s, RemoteRole=%s, HasAuthority=%d, IsLocallyControlled=%d"),
		*GetNameSafe(this),
		*LocalRoleStr,
		*RemoteRoleStr,
		HasAuthority() ? 1 : 0,
		IsLocallyControlled() ? 1 : 0
	);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
}

void ANetCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	FString LocalRoleStr = StaticEnum<ENetRole>()->GetNameStringByValue(GetLocalRole());
	FString RemoteRoleStr = StaticEnum<ENetRole>()->GetNameStringByValue(GetRemoteRole());

	const FString Msg = FString::Printf(
		TEXT("Actor=%s LocalRole=%s, RemoteRole=%s, HasAuthority=%d, IsLocallyControlled=%d"),
		*GetNameSafe(this),
		*LocalRoleStr,
		*RemoteRoleStr,
		HasAuthority() ? 1 : 0,
		IsLocallyControlled() ? 1 : 0
	);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
}

void ANetCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ANetCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ANetCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANetCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANetCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ANetCharacter::LookInput);

		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ANetCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ANetCharacter::DoStopFiring);
	}
	else
	{
		UE_LOG(LogArena, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ANetCharacter::MoveInput(const FInputActionValue& Value)
{
	if (IsDead())
	{
		return;
	}

	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();
	
	GEngine->AddOnScreenDebugMessage(4, 2.0f, FColor::Green, FString::Printf(TEXT("Move Input Value: %.2f %.2f"), MovementVector.X, MovementVector.Y));

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ANetCharacter::LookInput(const FInputActionValue& Value)
{
	if (IsDead())
	{
		return;
	}

	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	GEngine->AddOnScreenDebugMessage(5, 2.0f, FColor::Blue, FString::Format(TEXT("Look Input Value: {0} {1}"), {LookAxisVector.X, LookAxisVector.Y}));

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ANetCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ANetCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ANetCharacter::DoJumpStart()
{
	if (IsDead())
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(6, 2.0f, FColor::Green, TEXT("Jump Start"));
	// pass Jump to the character
	Jump();
}

void ANetCharacter::DoJumpEnd()
{
	GEngine->AddOnScreenDebugMessage(6, 2.0f, FColor::Red, TEXT("Jump End"));
	// pass StopJumping to the character
	StopJumping();
}

void ANetCharacter::DoStartFiring()
{
	if (IsDead())
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(8, 2.0f, FColor::Green, TEXT("Start Firing"));

	if (HasAuthority())
	{
		HandleStartFiring();
	}
	else
	{
		ServerStartFiring();
	}
}

void ANetCharacter::OnDeath()
{
	DoStopFiring();
	GetCharacterMovement()->DisableMovement();
}

void ANetCharacter::ServerStartFiring_Implementation()
{
	HandleStartFiring();
}

void ANetCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetCharacter, CurrentRifle);
}

void ANetCharacter::OnRep_CurrentRifle()
{
	OnCurrentRifleChanged.Broadcast(CurrentRifle);
}

void ANetCharacter::ServerStopFiring_Implementation()
{
	HandleStopFiring();
}

void ANetCharacter::HandleStopFiring()
{
	if (CurrentRifle)
	{
		CurrentRifle->StopFiringOnServer();
	}
}

float ANetCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || !HealthComponent)
	{
		return AppliedDamage;
	}

	return HealthComponent->ApplyHealthDamage(Damage);
}

bool ANetCharacter::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

void ANetCharacter::DoStopFiring()
{
	GEngine->AddOnScreenDebugMessage(8, 2.0f, FColor::Red, TEXT("End Firing"));

	if (HasAuthority())
	{
		HandleStopFiring();
	}
	else
	{
		ServerStopFiring();
	}
}

void ANetCharacter::HandleStartFiring()
{
	if (!IsDead() && CurrentRifle)
	{
		CurrentRifle->StartFiringOnServer();
	}
}
