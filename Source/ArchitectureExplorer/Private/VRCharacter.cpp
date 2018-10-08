// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//Create VR Root Scene Component for handling movement inside play space
	VRRoot = CreateDefaultSubobject<USceneComponent>(FName("VR Root"));
	VRRoot->SetupAttachment(GetRootComponent());
	//Adjust HMD position for Oculus Rift
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	VRRoot->SetRelativeLocation(FVector(0, 0, (GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) * -1));
	//Add camera component for the HMD to use
	Camera = CreateDefaultSubobject<UCameraComponent>(FName("Camera"));
	Camera->SetupAttachment(VRRoot);
	
	
	// Create VR Controllers.
	//RightMotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	//RightMotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	//RightMotionController->SetupAttachment(VRRoot);
	//L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	//L_MotionController->SetupAttachment(VRRoot);
	
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
	FVector CameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	CameraOffset.Z = 0;
	AddActorWorldOffset(CameraOffset);
	VRRoot->AddWorldOffset(-CameraOffset);
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(FName("MoveForward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(FName("StrafeRight"), this, &AVRCharacter::StrafeRight);
}

void AVRCharacter::MoveForward(float Scalar)
{
	AddMovementInput(Camera->GetForwardVector(), MoveSpeed * Scalar);
}

void AVRCharacter::StrafeRight(float Scalar)
{
	AddMovementInput(Camera->GetRightVector(), MoveSpeed * Scalar);
}

