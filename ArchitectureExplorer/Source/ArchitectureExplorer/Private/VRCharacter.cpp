// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AI/Navigation/NavigationSystem.h"
//#include "HeadMountedDisplayFunctionLibrary.h"

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
	FHitResult OutHit;
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * MaxTeleportDistance;
	bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECollisionChannel::ECC_Visibility);
	if (bHit == true) {
		TeleportMarker->SetWorldLocation(OutHit.Location);
		TeleportMarker->SetVisibility(true);
	}
	TeleportMarker->SetVisibility(bHit);

	FNavLocation FNav;
	//GetWorld()->GetNavigationSystem()->ProjectPointToNavigation(OutHit.Location, FNav, TeleportRange);
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
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr) {
		PC->PlayerCameraManager->StartCameraFade(0, 1, TeleportFadeTime, FLinearColor::Black);
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime, false);
}

void AVRCharacter::FinishTeleport()
{
	SetActorLocation(TeleportMarker->GetComponentLocation());
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr) {
		PC->PlayerCameraManager->StartCameraFade(1, 0, TeleportFadeTime, FLinearColor::Black);
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