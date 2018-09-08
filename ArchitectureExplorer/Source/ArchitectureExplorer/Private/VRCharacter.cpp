// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SplineComponent.h"
#include "Curves/CurveFloat.h"
#include "MotionControllerComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VR Root"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Left Controller"));
	LeftController->SetupAttachment(VRRoot);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Right Controller"));
	RightController->SetupAttachment(VRRoot);
	
	TeleportSpline = CreateDefaultSubobject<USplineComponent>(TEXT("Teleport beam"));
	TeleportSpline->SetupAttachment(RightController);

	TeleportMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Teleporter"));
	TeleportMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	TeleportMarker->SetVisibility(false);
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	if (BlinkerMaterialParent != nullptr) {
		BlinkerMaterial = UMaterialInstanceDynamic::Create(BlinkerMaterialParent, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterial);
		BlinkerMaterial->SetScalarParameterValue(TEXT("Radius"), 0);
	}
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CompensateForVRMovement();
	UpdateTeleportMarker();	
	SetVignetteRadiusDynamically();
}

bool AVRCharacter::FindTeleportDestination(TArray<FVector>& OutPath, FVector& OutLocation)
{
	FNavLocation NavLocation;
	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();

	FPredictProjectilePathParams Params(
		TeleportRadius, 
		Start, 
		Look * TeleportSpeed,
		SimTime, 
		ECollisionChannel::ECC_Visibility, 
		this
	);
	Params.bTraceComplex = true;
	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	if (!bHit) return false;
	for (FPredictProjectilePathPointData PointData : Result.PathData) {
		OutPath.Add(PointData.Location);
	}
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	bool bOnNavMesh = NavSystem->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportRange);
	if (!bOnNavMesh) return false;
	OutLocation = Result.HitResult.Location;
	return true;
}

void AVRCharacter::UpdateTeleportMarker()
{
	TArray<FVector> Path;
	FVector OutLocation;
	bool bHasDestination = FindTeleportDestination(Path, OutLocation);
	if (bHasDestination) {
		TeleportMarker->SetWorldLocation(OutLocation);
		TeleportMarker->SetVisibility(true);
		DrawTeleportPath(Path);
	}
	else {
		TeleportMarker->SetVisibility(false);
	}	
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector> &Path)
{
	UpdateSpline(Path);
	for (int32 i = 0; i < Path.Num(); ++i) {
		if (TeleportPathMeshPool.Num() <= i) {
			UStaticMeshComponent* DynamicMesh = NewObject<UStaticMeshComponent>(this);
			DynamicMesh->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			DynamicMesh->SetStaticMesh(TeleportMesh);
			DynamicMesh->SetMaterial(0, TeleportMaterial);
			DynamicMesh->RegisterComponent();
			TeleportPathMeshPool.Add(DynamicMesh);
		}
		UStaticMeshComponent* DynamicMesh = TeleportPathMeshPool[i];
		DynamicMesh->SetWorldLocation(Path[i]);
	}
}

void AVRCharacter::UpdateSpline(const TArray<FVector> &Path)
{
	TeleportSpline->ClearSplinePoints(false);
	for (int32 i = 0; i < Path.Num(); ++i) {
		FVector LocalPosition = TeleportSpline->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportSpline->AddPoint(Point, false);
	}
	TeleportSpline->UpdateSpline();
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

void AVRCharacter::SetVignetteRadiusDynamically()
{
	if (!RadiusVsVelocity) return;
	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);
	BlinkerMaterial->SetScalarParameterValue(TEXT("Radius"), Radius);
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