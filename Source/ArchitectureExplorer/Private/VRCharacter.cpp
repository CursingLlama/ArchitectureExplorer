// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Engine/World.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
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

	//Add camera component for the HMD to use
	Camera = CreateDefaultSubobject<UCameraComponent>(FName("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(FName("Destination Marker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(FName("Post Process Component"));
	PostProcessComponent->SetupAttachment(GetRootComponent());

	// Create VR Controllers.
	RightMotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	RightMotionController->SetTrackingMotionSource(FXRMotionControllerBase::RightHandSourceId);
	RightMotionController->SetShowDeviceModel(true);
	RightMotionController->SetupAttachment(VRRoot);
	
	LeftMotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	LeftMotionController->SetTrackingMotionSource(FXRMotionControllerBase::LeftHandSourceId);
	LeftMotionController->SetShowDeviceModel(true);
	LeftMotionController->SetupAttachment(VRRoot);
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

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Adjust HMD position for Oculus Rift TO DO: check for nullptr issue with first line
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	VRRoot->SetRelativeLocation(FVector(0, 0, (GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * -1));

	DestinationMarker->SetVisibility(false);

	if (BlinderParentMaterial)
	{
		BlinderInstance = UMaterialInstanceDynamic::Create(BlinderParentMaterial, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinderInstance);

		
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

	UpdateBlinder();

	UpdateDestinationMarker();
}

void AVRCharacter::UpdateBlinder()
{
	if (BlinderInstance && RadiusVsVelocity)
	{
		BlinderInstance->SetScalarParameterValue(FName("Radius"), RadiusVsVelocity->GetFloatValue(GetVelocity().Size()));
		FVector2D Center = GetBlinderCenter();
		BlinderInstance->SetVectorParameterValue(FName("Center"), FLinearColor(Center.X, Center.Y, 0));
	}	
}

FVector2D AVRCharacter::GetBlinderCenter()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	FVector MoveDirection = GetVelocity().GetSafeNormal();
	if (PlayerController || MoveDirection.IsNearlyZero())
	{
		FVector WorldStationaryPoint;
		if (FVector::DotProduct(Camera->GetForwardVector(), MoveDirection) > 0)
		{
			WorldStationaryPoint = Camera->GetComponentLocation() + MoveDirection * 100;
		}
		else
		{
			WorldStationaryPoint = Camera->GetComponentLocation() - MoveDirection * 100;
		}
		
		FVector2D ScreenPoint;
		PlayerController->ProjectWorldLocationToScreen(WorldStationaryPoint, ScreenPoint);

		int32 ScreenX, ScreenY;
		PlayerController->GetViewportSize(ScreenX, ScreenY);

		float NewX = FMath::Clamp(ScreenPoint.X / ScreenX, 0.2f, 0.8f);
		float NewY = ScreenPoint.Y / ScreenY;
		return FVector2D(NewX, NewY);
	}
		
	return FVector2D(0.5, 0.5);
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

bool AVRCharacter::FindTeleportLocation(FVector & OutLocation)
{
	FVector Start = LeftMotionController->GetComponentLocation() + LeftMotionController->GetForwardVector();
	float Gravity = FMath::Abs(GetWorld()->GetGravityZ());
	float Speed = FMath::Sqrt(Gravity * MaxTeleportDistance);
	float SimTime = 2.1 * (Speed / Gravity);
	FVector Velocity = LeftMotionController->GetForwardVector() * Speed;
	
	FPredictProjectilePathParams PathParams = FPredictProjectilePathParams(TeleportPathRadius, Start, Velocity, SimTime, ECC_Camera, this);
	///PathParams.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	FPredictProjectilePathResult PathResult;
	
	bool bHit = UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, PathResult);
	if (!bHit) return false;

	FNavLocation NavLocation;
	bool bProjected = GetWorld()->GetNavigationSystem()->ProjectPointToNavigation(PathResult.HitResult.Location, NavLocation, FVector(TeleportPathRadius));
	if (!bProjected) return false;

	OutLocation = NavLocation.Location;
	return true;
}

void AVRCharacter::BeginTeleport()
{
	if (!DestinationMarker->IsVisible()) return;
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
