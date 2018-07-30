// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

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
	bool bTeleportTarget = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		Start,
		End,
		ECollisionChannel::ECC_Visibility);
	if (bTeleportTarget == true) {
		TeleportMarker->SetWorldLocation(OutHit.Location);
	}
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
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}