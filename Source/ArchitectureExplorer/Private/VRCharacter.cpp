// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Engine/World.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

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

	//Adjust HMD position for Oculus Rift TO DO: check for nullptr issue with first line
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	VRRoot->SetRelativeLocation(FVector(0, 0, (GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) * -1));

	//Add camera component for the HMD to use
	Camera = CreateDefaultSubobject<UCameraComponent>(FName("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(FName("Destination Marker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(FName("Post Process Component"));
	PostProcessComponent->SetupAttachment(GetRootComponent());

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
	DestinationMarker->SetVisibility(false);

	if (BlinderParentMaterial)
	{
		BlinderInstance = UMaterialInstanceDynamic::Create(BlinderParentMaterial, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinderInstance);

		BlinderInstance->SetScalarParameterValue(FName("Radius"), 0.4f);
	}
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

bool AVRCharacter::FindTeleportLocation(FVector & OutLocation)
{
	FVector Start = LeftMotionController->GetComponentLocation() + LeftMotionController->GetForwardVector() * 5.f;
	FVector End = Start + LeftMotionController->GetForwardVector() * MaxTeleportDistance;
	FHitResult Hit;

	///DrawDebugLine(GetWorld(), Start, End, FColor::Green);
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);
	if (!bHit) return false;

	FNavLocation NavLocation;
	bool bProjected = GetWorld()->GetNavigationSystem()->ProjectPointToNavigation(Hit.Location, NavLocation, TeleportProjectionExtents);
	if (!bProjected) return false;

	OutLocation = NavLocation.Location;
	return true;
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector TeleLocation;

	if (FindTeleportLocation(TeleLocation))
	{
		DestinationMarker->SetWorldLocation(TeleLocation);
		DestinationMarker->SetVisibility(true);
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
	PlayerInputComponent->BindAction(FName("Teleport"), EInputEvent::IE_Released, this, &AVRCharacter::BeginTeleport);
}

void AVRCharacter::MoveForward(float Scalar)
{
	AddMovementInput(Camera->GetForwardVector(), MoveSpeed * Scalar);
}

void AVRCharacter::StrafeRight(float Scalar)
{
	AddMovementInput(Camera->GetRightVector(), MoveSpeed * Scalar);
}

void AVRCharacter::BeginTeleport()
{
	//Store teleport location so it doesn't change between Begin and Finish Teleport
	TeleportLocation = DestinationMarker->GetComponentLocation() + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	StartFade(0, 1);
	//Start timer to finish teleport once fade is complete
	FTimerHandle TeleportTimer;
	GetWorldTimerManager().SetTimer(TeleportTimer, this, &AVRCharacter::FinishTeleport, TeleportFadeTime);
}

void AVRCharacter::FinishTeleport()
{
	SetActorLocation(TeleportLocation);
	StartFade(1, 0);
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}
