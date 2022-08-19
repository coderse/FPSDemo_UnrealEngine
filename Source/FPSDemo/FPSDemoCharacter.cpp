// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSDemoCharacter.h"
#include "FPSDemoProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "GameFramework/CharacterMovementComponent.h"



DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AFPSDemoCharacter

AFPSDemoCharacter::AFPSDemoCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;

	//set the default player variables.
	isSprinting = false;
	isZoomedIn = false;
	weapon = nullptr;

	m4Ammo = 20;

}

void AFPSDemoCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFPSDemoCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFPSDemoCharacter::OnFire);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AFPSDemoCharacter::ManualReload);
	PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &AFPSDemoCharacter::ZoomIn);
	PlayerInputComponent->BindAction("ZoomIn", IE_Released, this, &AFPSDemoCharacter::StopZoom);

	//Add Switch
	PlayerInputComponent->BindAction("Switch", IE_Pressed, this, &AFPSDemoCharacter::SwitchCharacter);


	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AFPSDemoCharacter::OnResetVR);

	//Add sprinting
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AFPSDemoCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AFPSDemoCharacter::StopSprinting);


	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSDemoCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSDemoCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFPSDemoCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFPSDemoCharacter::LookUpAtRate);
}

void AFPSDemoCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			if (weapon)
			{
				if (weapon->clipAmmo > 0)
				{

					if (bUsingMotionControllers)
					{
						const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
						const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
						World->SpawnActor<AFPSDemoProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
					}
					else
					{
						const FRotator SpawnRotation = GetControlRotation();
						// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
						const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

						//Set Spawn Collision Handling Override
						FActorSpawnParameters ActorSpawnParams;
						ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

						// spawn the projectile at the muzzle
						World->SpawnActor<AFPSDemoProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
					}

					weapon->clipAmmo -= 1;
					// try and play the sound if specified
					if (FireSound != nullptr)
					{
						UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
					}

					// try and play a firing animation if specified
					if (FireAnimation != nullptr)
					{
						// Get the animation object for the arms mesh
						UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
						if (AnimInstance != nullptr)
						{
							AnimInstance->Montage_Play(FireAnimation, 1.f);
						}
					}

				}
				else
				{
					ReloadWeapon(weapon->weaponType);
				}
			


			}

		}
	}

	
}

void AFPSDemoCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AFPSDemoCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AFPSDemoCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void AFPSDemoCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void AFPSDemoCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFPSDemoCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFPSDemoCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFPSDemoCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AFPSDemoCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AFPSDemoCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AFPSDemoCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AFPSDemoCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}

void AFPSDemoCharacter::Sprint()
{
	if (auto characterMovement = GetCharacterMovement())
	{
		characterMovement->MaxWalkSpeed = 1500.0f;
		isSprinting = true;
	}
}

void AFPSDemoCharacter::StopSprinting()
{
	if (auto characterMovement = GetCharacterMovement())
	{
		characterMovement->MaxWalkSpeed = 600.0f;
		isSprinting = false;
	}
}

void AFPSDemoCharacter::ZoomIn()
{
	if (auto firsPersonCamera = GetFirstPersonCameraComponent())
	{
		firsPersonCamera->SetFieldOfView(60.0f);
		isZoomedIn = true;
	}
}

void AFPSDemoCharacter::StopZoom()
{
	if (auto firstPersonCamera = GetFirstPersonCameraComponent())
	{
		firstPersonCamera->SetFieldOfView(90.0f);
		isZoomedIn = false;
	}
}

void AFPSDemoCharacter::ManualReload() 
{
	if (weapon)
	{
		ReloadWeapon(weapon->weaponType);

	}
}


void AFPSDemoCharacter::ReloadWeapon(EWeaponType _weaponType) 
{
	if (weapon)
	{
		switch (_weaponType)
		{
		case EWeaponType::E_WeaponM4:
			m4Ammo = CalculateAmmo(m4Ammo);
			break;
		default:
			break;
		}
	}
}

int AFPSDemoCharacter::CalculateAmmo(int _ammoAmount)
{

	if (weapon->clipAmmo != weapon->maxClipAmmo)
	{
		if (_ammoAmount - (weapon->maxClipAmmo - weapon->clipAmmo) >= 0)
		{
			_ammoAmount -= (weapon->maxClipAmmo - weapon->clipAmmo);
			weapon->clipAmmo = weapon->maxClipAmmo;

		}
		else
		{
			weapon->clipAmmo += _ammoAmount;
			_ammoAmount = 0;
		}

	}
	else
	{
		TriggerOutOfAmmoPopUp();
	}
	return _ammoAmount;
}



void AFPSDemoCharacter::SwitchCharacter()
{
	if (otherCharater && GetController())
	{
		AController* temp = GetController();

		if (temp)
		{
			temp->UnPossess();
			temp->Possess(otherCharater);

		}

	}
}

void AFPSDemoCharacter::AddAmmo(EAmmoType _ammoType, int _ammoAmount)
{
	switch (_ammoType)
	{
	case EAmmoType::E_WeaponM4:
		m4Ammo += _ammoAmount;
		break;
	default:
		break;
	}
}




