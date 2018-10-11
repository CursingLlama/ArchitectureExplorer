// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "HandController.h"
#include "Engine/World.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

#include "HeadMountedDisplayFunctionLibrary.h"
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

	TeleportPath = CreateDefaultSubobject<USplineComponent>(FName("Teleportation Path"));
	TeleportPath->SetupAttachment(VRRoot);
	
	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(FName("Post Process Component"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
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

	LeftMotionController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (LeftMotionController)
	{
		LeftMotionController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftMotionController->SetHand(FXRMotionControllerBase::LeftHandSourceId);
	}

	RightMotionController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (RightMotionController)
	{
		RightMotionController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightMotionController->SetHand(FXRMotionControllerBase::RightHandSourceId);
	}

	LeftMotionController->PairController(RightMotionController);
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(FName("MoveForward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(FName("StrafeRight"), this, &AVRCharacter::StrafeRight);
	PlayerInputComponent->BindAction(FName("Teleport"), EInputEvent::IE_Released, this, &AVRCharacter::BeginTeleport);

	PlayerInputComponent->BindAction(FName("GripLeft"), EInputEvent::IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction(FName("GripLeft"), EInputEvent::IE_Released, this, &AVRCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction(FName("GripRight"), EInputEvent::IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction(FName("GripRight"), EInputEvent::IE_Released, this, &AVRCharacter::ReleaseRight);

}

void AVRCharacter::MoveForward(float Scalar)
{
	AddMovementInput(Camera->GetForwardVector(), MoveSpeed * Scalar);
}

void AVRCharacter::StrafeRight(float Scalar)
{
	AddMovementInput(Camera->GetRightVector(), MoveSpeed * Scalar);
}

void AVRCharacter::GripLeft() {	LeftMotionController->Grip(); }
void AVRCharacter::ReleaseLeft() { LeftMotionController->Release(); }
void AVRCharacter::GripRight() { RightMotionController->Grip(); }
void AVRCharacter::ReleaseRight() { RightMotionController->Release(); }

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//Adjust character position to match the Movement of the HMD inside it's play space
	FVector CameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	CameraOffset.Z = 0;
	AddActorWorldOffset(CameraOffset);
	VRRoot->AddWorldOffset(-CameraOffset);

	//UpdateBlinder();

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
	if (PlayerController && !MoveDirection.IsNearlyZero())
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
	FVector Start = LeftMotionController->GetActorLocation() + LeftMotionController->GetActorForwardVector();
	float Gravity = FMath::Abs(GetWorld()->GetGravityZ());
	float Speed = FMath::Sqrt(Gravity * MaxTeleportDistance);
	float SimTime = 2.1 * (Speed / Gravity);
	FVector Velocity = LeftMotionController->GetActorForwardVector() * Speed;
	
	FPredictProjectilePathParams PathParams = FPredictProjectilePathParams(TeleportPathRadius, Start, Velocity, SimTime, ECC_Camera, this);
	///PathParams.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	FPredictProjectilePathResult PathResult;
	
	bool bHit = UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, PathResult);

	UpdateTeleportPath(PathResult.PathData);
	
	if (!bHit) return false;
		
	FNavLocation NavLocation;
	bool bProjected = GetWorld()->GetNavigationSystem()->ProjectPointToNavigation(PathResult.HitResult.Location, NavLocation, FVector(TeleportPathRadius));
	if (!bProjected) return false;

	OutLocation = NavLocation.Location;
	return true;
}

void AVRCharacter::UpdateTeleportPath(const TArray<FPredictProjectilePathPointData>& Path)
{
	
	TArray<FVector> VectorPath;
	for (int32 i = 0; i < Path.Num(); i++)
	{
		VectorPath.Emplace(Path[i].Location);
	}
	TeleportPath->SetSplinePoints(VectorPath, ESplineCoordinateSpace::World);

	for (int32 i = 0; i < Path.Num() - 1; i++)
	{
		if (PathMeshPool.Num() <= i)
		{
			USplineMeshComponent* NewDynamicMesh = NewObject<USplineMeshComponent>(this);
			NewDynamicMesh->SetMobility(EComponentMobility::Movable);
			NewDynamicMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			NewDynamicMesh->SetStaticMesh(TeleportPathMesh);
			NewDynamicMesh->SetMaterial(0, TeleportPathMaterial);
			NewDynamicMesh->RegisterComponent();
			PathMeshPool.Emplace(NewDynamicMesh);
		}
		
		USplineMeshComponent* DynamicMesh = PathMeshPool[i];
		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);
		DynamicMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		DynamicMesh->SetVisibility(true);
	}
	for (int32 i = Path.Num() - 1; i < PathMeshPool.Num(); i++)
	{
		PathMeshPool[i]->SetVisibility(false);
	}
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
