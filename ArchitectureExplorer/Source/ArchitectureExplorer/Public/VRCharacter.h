// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter {
	GENERATED_BODY()

		//testing 1 2
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Sets default values for this character's properties
	AVRCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere)
		class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
		class AHandController* LeftController = nullptr;

	UPROPERTY(VisibleAnywhere)
		class AHandController* RightController = nullptr;

	UPROPERTY(VisibleAnywhere)
		class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* TeleportMarker;

	UPROPERTY(VisibleAnywhere)
		class USplineComponent* TeleportSpline;

	UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocity = nullptr;

	UPROPERTY(VisibleAnywhere)
		class USplineComponent* TeleportPath;

	UPROPERTY()
		TArray<class USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(EditDefaultsOnly)
		class UStaticMesh* TeleportMesh = nullptr;

	UPROPERTY(EditDefaultsOnly)
		class UMaterialInterface* TeleportMaterial = nullptr;

	UPROPERTY()
		class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
		float TeleportRadius = 20;

	UPROPERTY(EditAnywhere)
		float TeleportSpeed = 500;

	UPROPERTY(EditAnywhere)
		float RotationDegrees = 45;

	UPROPERTY(EditAnywhere)
		float SimTime = 5;

	UPROPERTY(EditAnywhere)
		float TeleportFadeTime = .2;

	UPROPERTY(EditAnywhere)
		FVector TeleportRange = FVector(100, 100, 100);

	UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinkerMaterialParent = nullptr;

	UPROPERTY()
		class UMaterialInstanceDynamic* BlinkerMaterial = nullptr;

private:

	void MoveForward(float throttle);
	void MoveRight(float throttle);
	void BeginTeleport();
	void FinishTeleport();
	void TurnRight();
	void TurnLeft();
	void Fade(float From, float To);
	void CompensateForVRMovement();
	void UpdateTeleportMarker();
	void DrawTeleportPath(const TArray<FVector>& Path);
	void UpdateSpline(const TArray<FVector>& Path);
	bool FindTeleportDestination(TArray<FVector>&, FVector&);
	void SetVignetteRadiusDynamically();
};