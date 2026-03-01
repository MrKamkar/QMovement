// QuakePawn.cpp — APawn using QMovement (GoldSrc pmove.c port)

#include "QuakePawn.h"
#include "UMovementSettings.h"

#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

// ============================================================================
// Constructor
// ============================================================================
AQuakePawn::AQuakePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule root — matches Quake player hull: {-16,-16,-24} to {16,16,32}
	// UE capsule: radius=16, halfHeight=28 (total height 56 = 24+32)
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(16.0f, 28.0f);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComp->SetSimulatePhysics(false);
	CapsuleComp->SetEnableGravity(false);
	SetRootComponent(CapsuleComp);

	// Camera at eye height (Quake viewheight = 22 from hull center)
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(CapsuleComp);
	CameraComp->SetRelativeLocation(FVector(0.0f, 0.0f, 22.0f));
	CameraComp->bUsePawnControlRotation = false;
}

// ============================================================================
// BeginPlay
// ============================================================================
void AQuakePawn::BeginPlay()
{
	Super::BeginPlay();

	// Initialize movement engine position from actor location
	MoveEngine.Origin = GetActorLocation();

	// If MovementSettings has custom capsule dims, apply them to the component
	if (MovementSettings)
	{
		CapsuleComp->SetCapsuleSize(MovementSettings->CapsuleRadius, MovementSettings->CapsuleHalfHeight);
	}
}

// ============================================================================
// Tick — the main loop
// ============================================================================
void AQuakePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// --- Build movement command ---

	// View angles from Controller rotation
	const FRotator ControlRot = PC->GetControlRotation();
	MoveEngine.ViewAngles.X = ControlRot.Pitch;
	MoveEngine.ViewAngles.Y = ControlRot.Yaw;
	MoveEngine.ViewAngles.Z = ControlRot.Roll;

	// Forward/side move: input * maxspeed
	// MaxSpeed is read from settings inside ProcessMovement, but we need it here for the command.
	// Use the settings value if available, otherwise fallback 250.
	const float CmdMaxSpeed = MovementSettings ? MovementSettings->MaxSpeed : 250.0f;
	MoveEngine.Cmd.ForwardMove = MoveInput.Y * CmdMaxSpeed;
	MoveEngine.Cmd.SideMove    = MoveInput.X * CmdMaxSpeed;
	MoveEngine.Cmd.UpMove      = 0.0f;
	MoveEngine.Cmd.Msec        = DeltaTime * 1000.0f;

	// Jump button
	if (bWantsJump)
		MoveEngine.Cmd.Buttons |= 2; // BUTTON_JUMP
	else
		MoveEngine.Cmd.Buttons &= ~2;

	// Duck button
	if (bWantsCrouch)
		MoveEngine.Cmd.Buttons |= 4; // BUTTON_DUCK
	else
		MoveEngine.Cmd.Buttons &= ~4;

	// --- Execute GoldSrc movement (pass Data Asset — may be nullptr) ---
	MoveEngine.ProcessMovement(DeltaTime, GetWorld(), MovementSettings, this);

	// --- Apply results ---
	SetActorLocation(MoveEngine.Origin, false, nullptr, ETeleportType::TeleportPhysics);

	float CurrentRadius = MovementSettings ? MovementSettings->CapsuleRadius : 16.0f;
	float CurrentHalfHeight = MoveEngine.bIsCrouching ? 
		(MovementSettings ? MovementSettings->CapsuleHalfHeightCrouch : 18.0f) : 
		(MovementSettings ? MovementSettings->CapsuleHalfHeight : 28.0f);

	CapsuleComp->SetCapsuleSize(CurrentRadius, CurrentHalfHeight, false);

	// Update camera rotation to match view angles
	CameraComp->SetWorldRotation(ControlRot);

	// --- Debug: show speed on screen ---
	if (bShowDebugSpeed)
	{
		const float HorizSpeed = FVector(MoveEngine.Velocity.X, MoveEngine.Velocity.Y, 0.0f).Size();
		const FString SpeedStr = FString::Printf(
			TEXT("Speed: %.1f  |  Vel: %.1f, %.1f, %.1f  |  Ground: %s"),
			HorizSpeed,
			MoveEngine.Velocity.X, MoveEngine.Velocity.Y, MoveEngine.Velocity.Z,
			MoveEngine.bOnGround ? TEXT("YES") : TEXT("NO")
		);
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, SpeedStr);

		// Debug: show raw input values
		const FString InputStr = FString::Printf(
			TEXT("Input: X=%.2f Y=%.2f  |  Jump=%s  |  Fwd=%.1f  Side=%.1f"),
			MoveInput.X, MoveInput.Y,
			bWantsJump ? TEXT("YES") : TEXT("NO"),
			MoveEngine.Cmd.ForwardMove, MoveEngine.Cmd.SideMove
		);
		GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Yellow, InputStr);
	}
}

// ============================================================================
// SetupPlayerInputComponent — Enhanced Input bindings
// ============================================================================
void AQuakePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// --- Add Enhanced Input Mapping Context ---
	// This is done HERE (not BeginPlay) because the Controller is guaranteed
	// to be valid at this point — SetupPlayerInputComponent is called AFTER possession.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (MappingContext)
			{
				Subsystem->AddMappingContext(MappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("QuakePawn: MappingContext added OK"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("QuakePawn: MappingContext is NULL! Assign IMC_Quake in Blueprint."));
			}
		}

		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	// --- Bind Enhanced Input Actions ---
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		UE_LOG(LogTemp, Log, TEXT("QuakePawn: EnhancedInputComponent found — binding actions"));

		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AQuakePawn::OnMove);
			EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AQuakePawn::OnMove);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("QuakePawn: MoveAction is NULL!"));
		}

		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AQuakePawn::OnLook);
		}
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AQuakePawn::OnJumpPressed);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AQuakePawn::OnJumpReleased);
		}
		if (DuckAction)
		{
			EIC->BindAction(DuckAction, ETriggerEvent::Triggered, this, &AQuakePawn::OnDuckPressed);
			EIC->BindAction(DuckAction, ETriggerEvent::Completed, this, &AQuakePawn::OnDuckReleased);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("QuakePawn: DuckAction is NULL!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("QuakePawn: InputComponent is NOT EnhancedInputComponent! WASD will not work."));
	}
}

// ============================================================================
// Input callbacks
// ============================================================================

void AQuakePawn::OnMove(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

void AQuakePawn::OnLook(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->AddYawInput(LookValue.X * MouseSensitivity);
		PC->AddPitchInput(-LookValue.Y * MouseSensitivity);
	}
}

void AQuakePawn::OnJumpPressed(const FInputActionValue& /*Value*/)
{
	bWantsJump = true;
}

void AQuakePawn::OnJumpReleased(const FInputActionValue& /*Value*/)
{
	bWantsJump = false;
}

void AQuakePawn::OnDuckPressed(const FInputActionValue& /*Value*/)
{
	bWantsCrouch = true;
}

void AQuakePawn::OnDuckReleased(const FInputActionValue& /*Value*/)
{
	bWantsCrouch = false;
}

// ============================================================================
// Console commands — like CS 1.6: type in UE console (~ key)
// Example: sv_airaccelerate 12
// ============================================================================

// Macro: if Args is empty → display current value; otherwise parse & set.
#define SV_CMD(Name, Field) \
void AQuakePawn::Name(const FString& Args) \
{ \
	const FString Trimmed = Args.TrimStartAndEnd(); \
	if (Trimmed.IsEmpty()) \
	{ \
		const float Current = MovementSettings ? MovementSettings->Field : 0.0f; \
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, \
			FString::Printf(TEXT(#Name " is \"%.2f\""), Current)); \
	} \
	else \
	{ \
		const float Value = FCString::Atof(*Trimmed); \
		if (MovementSettings) { MovementSettings->Field = Value; } \
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, \
			FString::Printf(TEXT(#Name " = %.2f"), Value)); \
	} \
}

SV_CMD(sv_gravity, Gravity)
SV_CMD(sv_maxspeed, MaxSpeed)
SV_CMD(sv_airaccelerate, AirAccelerate)
SV_CMD(sv_accelerate, Accelerate)
SV_CMD(sv_friction, Friction)
SV_CMD(sv_jumpspeed, JumpSpeed)
SV_CMD(sv_stopspeed, StopSpeed)
SV_CMD(sv_stepsize, StepSize)
SV_CMD(sv_airspeedcap, AirSpeedCap)
SV_CMD(sv_maxvelocity, MaxVelocity)

#undef SV_CMD

void AQuakePawn::sv_autobhop(const FString& Args)
{
	const FString Trimmed = Args.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		const bool Current = MovementSettings ? MovementSettings->bAutoBunnyHop : false;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
			FString::Printf(TEXT("sv_autobhop is \"%d\""), Current ? 1 : 0));
	}
	else
	{
		const bool Value = (FCString::Atoi(*Trimmed) != 0);
		if (MovementSettings) { MovementSettings->bAutoBunnyHop = Value; }
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("sv_autobhop = %d"), Value ? 1 : 0));
	}
}

void AQuakePawn::noclip()
{
	MoveEngine.bSpectator = !MoveEngine.bSpectator;

	if (MoveEngine.bSpectator)
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("noclip ON"));
	}
	else
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("noclip OFF"));
	}
}
