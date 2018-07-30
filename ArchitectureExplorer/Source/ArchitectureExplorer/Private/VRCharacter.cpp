// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "NavigationSystem.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VR Root"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	TeleportMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Teleporter"));
	TeleportMarker->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CompensateForVRMovement();
	UpdateTeleportMarker();		
}

void AVRCharacter::UpdateTeleportMarker()
{
	FVector OutLocation;
	bool bHit = FindTeleportDestination(OutLocation);
	if (bHit) {
		TeleportMarker->SetWorldLocation(OutLocation);
		TeleportMarker->SetVisibility(true);
	}
	TeleportMarker->SetVisibility(bHit);
}

bool AVRCharacter::FindTeleportDestination(FVector& OutLocation)
{
	FHitResult OutHit;
	FNavLocation NavLocation;
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * MaxTeleportDistance;

	bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECollisionChannel::ECC_Visibility);
	if (!bHit) return false;

	bool bOnNavMesh = UNavigationSystemV1::GetNavigationSystem(GetWorld())->ProjectPointToNavigation(OutHit.Location, NavLocation, TeleportRange);
	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;
	return true;
}

void AVRCharacter::CompensateForVRMovement()
{
	FVector CameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	CameraOffset.Z = 0;
	AddActorWorldOffset(CameraOffset);
	VRRoot->AddWorldOffset(-CameraOffset);
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), EInputEvent::IE_Released, this, &AVRCharacter::BeginTeleport);
	//PlayerInputComponent->BindAction(TEXT("RotateRight"), EInputEvent::IE_Pressed, this, &AVRCharacter::TurnRight);
	//PlayerInputComponent->BindAction(TEXT("RotateLeft"), EInputEvent::IE_Pressed, this, &AVRCharacter::TurnLeft);
}

void AVRCharacter::BeginTeleport()
{
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime, false);
	Fade(0, 1);
}

void AVRCharacter::FinishTeleport()
{
	SetActorLocation(TeleportMarker->GetComponentLocation());
	SetActorLocation(TeleportMarker->GetComponentLocation() + GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	Fade(1, 0);
}

void AVRCharacter::Fade(float From, float To)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr) {
		PC->PlayerCameraManager->StartCameraFade(From, To, TeleportFadeTime, FLinearColor::Black);
	}
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::TurnRight()
{
	FRotator Turn;
	Turn.Yaw = RotationDegrees;
	VRRoot->AddWorldRotation(Turn);
}

void AVRCharacter::TurnLeft()
{
	FRotator Turn;
	Turn.Yaw = -RotationDegrees;
	VRRoot->AddWorldRotation(Turn);
}