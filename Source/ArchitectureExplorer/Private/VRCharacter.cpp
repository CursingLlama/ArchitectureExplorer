// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "DrawDebugHelpers.h"


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

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(FName("Destination Marker"));
	DestinationMarker->SetupAttachment(GetRootComponent());
	
	
	// Create VR Controllers.
	RightMotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	RightMotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	RightMotionController->SetupAttachment(VRRoot);
	RightHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Right Hand Mesh"));
	RightHandMesh->SetupAttachment(RightMotionController);
	
	LeftMotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	LeftMotionController->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
	LeftMotionController->SetupAttachment(VRRoot);
	LeftHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Left Hand Mesh"));
	LeftHandMesh->SetupAttachment(LeftMotionController);
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
	//Adjust character position to match the Movement of the HMD inside it's play space
	FVector CameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	CameraOffset.Z = 0;
	AddActorWorldOffset(CameraOffset);
	VRRoot->AddWorldOffset(-CameraOffset);

	UpdateDestinationMarker();
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector Start = LeftMotionController->GetComponentLocation() + LeftMotionController->GetForwardVector() * 5.f;
	FVector End = Start + LeftMotionController->GetForwardVector() * MaxTeleportDistance;
	FHitResult Hit;
	DrawDebugLine(GetWorld(), Start, End, FColor::Green);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(Hit.Location);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
	}
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

