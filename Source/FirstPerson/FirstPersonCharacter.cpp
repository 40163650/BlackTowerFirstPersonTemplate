// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonCharacter.h"
#include "FirstPersonProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AFirstPersonCharacter

// While the blink key is held
// Cast a ray from the player camera to the teleport point
// where it is aimed, set that as the teleport destination
// Show the destination as a physical object
// Bonus: collision
void AFirstPersonCharacter::UBlinkStart()
{
	if (blinkCoolDown == 0)
	{
		bIsBlinking = true;
		TeleportIndicatorComponent->SetVisibility(true);
	}
}

// Fade to black,
// Teleport player,
// Fade from black,
// Start cooldown timer
void AFirstPersonCharacter::UBlinkComplete()
{
	if (bIsBlinking)
	{
		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
		FLinearColor FadeColor(0.0f, 0.0f, 0.0f);
		CameraManager->StartCameraFade(0.0f, 1.0f, 0.1f, FadeColor, false /*fadeAudio?*/, true /*holdWhenFinished?*/);
		TeleportTo(blinkDestination, GetWorld()->GetFirstPlayerController()->GetControlRotation(), false /*isATest?*/, true /*noCheck?*/);
		CameraManager->StartCameraFade(1.0f, 0.0f, 0.1f, FadeColor, false /*fadeAudio?*/, true /*holdWhenFinished?*/);
		blinkCoolDown = blinkCoolDownSeconds;
		bIsBlinking = false;
		TeleportIndicatorComponent->SetVisibility(false);
	}
}

AFirstPersonCharacter::AFirstPersonCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	TeleportIndicatorComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportIndicator"));
	TeleportIndicatorComponent->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshAsset(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere'"));
	TeleportIndicatorComponent->SetStaticMesh(MeshAsset.Object);
	TeleportIndicatorComponent->SetWorldScale3D(FVector(0.1f, 0.1f, 0.1f));
	TeleportIndicatorComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void AFirstPersonCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void AFirstPersonCharacter::Tick(float DeltaSeconds)
{
	// Call the base class
	Super::Tick(DeltaSeconds);

	// If we've begun blinking
	// Show where we're going to go
	if (bIsBlinking)
	{
		FVector CharacterLocation = GetActorLocation();
		FHitResult HitResult;
		FVector EndLocation = CharacterLocation + 10000 * UKismetMathLibrary::GetForwardVector(GetWorld()->GetFirstPlayerController()->GetControlRotation());
		if (UKismetSystemLibrary::LineTraceSingle(GetWorld(), CharacterLocation, EndLocation, ETraceTypeQuery::TraceTypeQuery1, false, TArray<class AActor*>(),	EDrawDebugTrace::None, HitResult, true))
		{
			blinkDestination = HitResult.Location;
			TeleportIndicatorComponent->SetWorldLocation(blinkDestination);
		}
	}

	if (blinkCoolDown > 0)
	{
		blinkCoolDown = FMath::Max(blinkCoolDown - DeltaSeconds, 0.0f);
		// TODO: Update UI to show coolDown level
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void AFirstPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFirstPersonCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFirstPersonCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AFirstPersonCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AFirstPersonCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}