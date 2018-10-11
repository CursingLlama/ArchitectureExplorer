// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	
	// Sets default values for this character's properties
	AVRCharacter();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
		   
private:

	UPROPERTY(EditDefaultsOnly) TSubclassOf<class AHandController> HandControllerClass;
	/** Motion controller (right hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true")) class AHandController* RightMotionController = nullptr;
	/** Motion controller (left hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true")) class AHandController* LeftMotionController = nullptr;
	/** A scene root for the VR components to offset movement in play space */
	UPROPERTY() class USceneComponent* VRRoot = nullptr;
	/** Camera to be controlled by HMD */
	UPROPERTY() class UCameraComponent* Camera = nullptr;

	/** Maximum distance for teleport locomotion */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport") float MaxTeleportDistance = 1500;
	/** Radius used for tracing teleportation path and the extents for projecting to the NavMesh */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport") float TeleportPathRadius = 34;
	/** Time in seconds for fade to black and back during teleportation */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport") float TeleportFadeTime = 0.75f;
	/** Stored location at begin teleport and used as the target destination on finish teleport */
	UPROPERTY() FVector TeleportLocation;

	/** Used to trace the teleportation path for rendering */
	UPROPERTY(VisibleAnywhere, Category = "Teleport") class USplineComponent* TeleportPath = nullptr;
	/** Mesh used to display the traced path of the teleport */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport") class UStaticMesh* TeleportPathMesh;
	/** Material applied to TeleportationPathMesh */
	UPROPERTY(EditDefaultsOnly, Category = "Teleport") class UMaterialInterface* TeleportPathMaterial;
	/** Mesh used to display the endpoint of the teleportation path when valid */
	UPROPERTY(VisibleAnywhere, Category = "Teleport") class UStaticMeshComponent* DestinationMarker = nullptr;
	UPROPERTY() TArray<class USplineMeshComponent*> PathMeshPool;
		
	/** Maximum speed for free locomotion */
	UPROPERTY(EditDefaultsOnly, Category = "Movement") float MoveSpeed = 10;
	/** Material used by post process to apply "blinders" to player vision during free locomotion */
	UPROPERTY(EditDefaultsOnly, Category = "Movement") class UMaterialInterface* BlinderParentMaterial = nullptr;
	/** Modifies how tight the blinders are based on velocity */
	UPROPERTY(EditDefaultsOnly, Category = "Movement") class UCurveFloat* RadiusVsVelocity = nullptr;
	/** Stores the dynamic instace of the blinder parent material */
	UPROPERTY() class UMaterialInstanceDynamic* BlinderInstance = nullptr;
	/** Post process effect used for blinders */
	UPROPERTY() class UPostProcessComponent* PostProcessComponent = nullptr;
	
	
	void UpdateBlinder();
	FVector2D GetBlinderCenter();
	void UpdateDestinationMarker();
	bool FindTeleportLocation(FVector &OutLocation);
	void UpdateTeleportPath(const TArray<struct FPredictProjectilePathPointData>& Path);
	void MoveForward(float Scalar);
	void StrafeRight(float Scalar);
	void BeginTeleport();
	void FinishTeleport();
	void StartFade(float FromAlpha, float ToAlpha);
};
