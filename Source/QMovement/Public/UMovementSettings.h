// UMovementSettings.h — GoldSrc movement parameters as UE5 Data Asset
//
// Create in Editor: Content Browser → Right-click → Miscellaneous → Data Asset → MovementSettings
// Assign to QuakePawn → MovementSettings slot. All values default to QuakeWorld.
// Console commands (sv_gravity, sv_airaccelerate, etc.) modify these values at runtime.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UMovementSettings.generated.h"

/**
 * Data Asset containing all GoldSrc movement parameters.
 * Defaults are QuakeWorld values. For CS 1.6: MaxSpeed=250, AirAccelerate=10, Friction=4, JumpSpeed=268.3.
 */
UCLASS(BlueprintType)
class QMOVEMENT_API UMovementSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	// =========================================================================
	// Speed
	// =========================================================================

	/** Maximum ground movement speed (HL = 320, CS 1.6 = 250) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float MaxSpeed = 320.0f;

	/** Jump velocity added on jump (QW = 270, CS 1.6 = 268.328) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float JumpSpeed = 268.328157f;

	/** Spectator fly speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float SpectatorMaxSpeed = 500.0f;

	/** Speed below which full stop friction kicks in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float StopSpeed = 100.0f;

	// =========================================================================
	// Physics
	// =========================================================================

	/** Gravity in units/s² (CS 1.6 = 800) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float Gravity = 800.0f;

	/** Per-entity gravity multiplier (1.0 = normal) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float EntGravity = 1.0f;

	/** Ground friction (QW = 6, CS 1.6 = 4) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float Friction = 4.0f;

	/** Ground acceleration (CS 1.6 = 10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float Accelerate = 10.0f;

	/** Air acceleration — THE key value for bhop/air-strafe (QW = 0.7, CS 1.6 = 10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float AirAccelerate = 10.0f;

	/** Water acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float WaterAccelerate = 10.0f;

	/** Water friction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float WaterFriction = 1.0f;

	/** Absolute max velocity clamp — prevents infinite speed exploits (CS 1.6 = 2000) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float MaxVelocity = 2000.0f;

	// =========================================================================
	// Geometry
	// =========================================================================

	/** Max step height the player can walk up (Quake = 18) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float StepSize = 18.0f;

	/** Capsule collision radius (Quake hull = 16) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float CapsuleRadius = 16.0f;

	/** Capsule collision half-height (Quake hull: 24+32 = 56 total → 28 half) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float CapsuleHalfHeight = 28.0f;

	/** Capsule collision half-height when crouching (CS: 36 total -> 18 half) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float CapsuleHalfHeightCrouch = 18.0f;

	/** Camera height relative to the player origin when standing (CS 1.6 = ~22-28) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float ViewHeight = 22.0f;

	/** Camera height relative to the player origin when crouching (CS 1.6 = ~9-12) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	float ViewHeightCrouch = 9.0f;

	// =========================================================================
	// Advanced
	// =========================================================================

	/** Max air-strafe wishspeed clamp (Quake hardcodes 30 — changing this value alters air-strafe feel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "1.0"))
	float AirSpeedCap = 30.0f;

	/** Minimum surface normal Z to count as ground (< this = too steep = slide) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GroundNormalZ = 0.7f;

	/** Max upward velocity to still count as on-ground (original = 180) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
	float MaxGroundUpVelocity = 180.0f;

	// =========================================================================
	// Gameplay
	// =========================================================================

	/** Auto-bunnyhop: hold Space to auto-jump on landing (no scroll-timing needed).
	 *  false = classic CS 1.6 (must release & re-press / scroll to jump again)
	 *  true  = auto-bhop (KZ/Surf servers, modern bhop maps) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	bool bAutoBunnyHop = false;

	/** Disable the default CS 1.6 / HL 1.7x bhop speed cap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	bool bDisableBhopCap = true;

	/** Number of jumps allowed before touching the ground (1 = normal, 2 = double jump) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay", meta = (ClampMin = "1"))
	int32 MaxJumps = 1;

	// =========================================================================
	// Camera Animations
	// =========================================================================

	/** Enable the CS 1.6 style landing camera punch (impact shake) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bEnableCameraPunch = true;

	/** Enable the CS 1.6 style strafe camera roll (leaning) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bEnableCameraRoll = true;

	/** Enable head-bobbing while moving */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bEnableBobbing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BobAmount = 0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BobSpeed = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BobUp = 0.5f;
};
