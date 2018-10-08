// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
//#include "HeadMountedDisplayFunctionLibrary.h"
//#include "MotionControllerComponent.h"
//#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(FName("Camera"));
	Camera->SetupAttachment(GetRootComponent());
	
	// Create VR Controllers.
	//R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	//R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	//R_MotionController->SetupAttachment(GetRootComponent());
	//L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	//L_MotionController->SetupAttachment(GetRootComponent());
	
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

