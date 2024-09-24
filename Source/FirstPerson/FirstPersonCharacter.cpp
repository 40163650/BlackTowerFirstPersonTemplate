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
// This is done in the tick, it is initiated when the key is pressed here
void AFirstPersonCharacter::UBlinkStart()
{
	if (BlinkCoolDown == 0)
	{
		bIsBlinking = true;
		TeleportIndicatorComponent->SetVisibility(true);
	}
}

// Fade to black,
// Teleport player,
// Fade from black,
// Start cooldown timer
// Play warp audio
void AFirstPersonCharacter::UBlinkComplete()
{
	if (bIsBlinking)
	{
		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
		FLinearColor FadeColor(0.0f, 0.0f, 0.0f);
		CameraManager->StartCameraFade(0.0f, 1.0f, 0.1f, FadeColor, false /*fadeAudio?*/, true /*holdWhenFinished?*/);
		UGameplayStatics::PlaySound2D(this, WarpWave);
		TeleportTo(BlinkDestination, GetWorld()->GetFirstPlayerController()->GetControlRotation(), false /*isATest?*/, true /*noCheck?*/);
		CameraManager->StartCameraFade(1.0f, 0.0f, 0.1f, FadeColor, false /*fadeAudio?*/, true /*holdWhenFinished?*/);
		BlinkCoolDown = BlinkCoolDownSeconds;
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
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// Create a mesh showing where we will teleport to when we are teleporting
	TeleportIndicatorComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportIndicator"));
	TeleportIndicatorComponent->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshAsset(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere'"));
	TeleportIndicatorComponent->SetStaticMesh(MeshAsset.Object);
	TeleportIndicatorComponent->SetWorldScale3D(FVector(0.1f, 0.1f, 0.1f));
	TeleportIndicatorComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create audio waves to play during teleportation and when we are ready to teleport again
	static ConstructorHelpers::FObjectFinder<USoundWave> warpResource(TEXT("SoundWave'/Game/Audio/warp.warp'"));
	WarpWave = warpResource.Object;

	static ConstructorHelpers::FObjectFinder<USoundWave> cooldownResource(TEXT("SoundWave'/Game/Audio/cooldownfinished.cooldownfinished'"));
	CooldownFinishedWave = cooldownResource.Object;
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
			BlinkDestination = HitResult.Location;
			TeleportIndicatorComponent->SetWorldLocation(BlinkDestination);
		}
	}

	// Tick down the cooldown timer, play audio when it finishes
	if (BlinkCoolDown > 0)
	{
		BlinkCoolDown = FMath::Max(BlinkCoolDown - DeltaSeconds, 0.0f);
		if (BlinkCoolDown == 0)
		{
			UGameplayStatics::PlaySound2D(this, CooldownFinishedWave);
		}
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