// QMovement.h — QuakeWorld pmove.c adapted to UE5 C++
// Original: Copyright (C) 1996-1997 Id Software, Inc. (GPL v2)
//
// Port of QuakeWorld pmove.c with the following adaptations:
//   - Types replaced: vec3_t → FVector, pmtrace_t → FQMoveTrace, usercmd_t → FQUserCmd
//   - Traces use UE5 SweepSingleByChannel (capsule shape) instead of BSP hull checks
//   - AirMove passes Sv_AirAccelerate (HL/CS 1.6) instead of movevars.accelerate (QW original)
//   - Auto-bunnyhop toggle added (skips OldButtons pogo-stick check)
//   - All parameters read from UMovementSettings Data Asset at runtime

#pragma once

#include "CoreMinimal.h"

class UMovementSettings;

// --------------------------------------------------------------------------
// FQMoveTrace — replaces pmtrace_t
// --------------------------------------------------------------------------
struct FQMoveTrace
{
	bool  bAllSolid   = false;   // if true, plane is not valid
	bool  bStartSolid = false;   // if true, initial point was in solid
	float Fraction    = 1.0f;    // time completed, 1.0 = didn't hit anything
	FVector EndPos    = FVector::ZeroVector;
	FVector PlaneNormal = FVector::ZeroVector;
	int32 Ent         = -1;      // entity the surface is on (-1 = none)
};

// --------------------------------------------------------------------------
// FQUserCmd — replaces usercmd_t (input from player per tick)
// --------------------------------------------------------------------------
struct FQUserCmd
{
	float ForwardMove = 0.0f;
	float SideMove    = 0.0f;
	float UpMove      = 0.0f;
	float Msec        = 0.0f;    // frame time in milliseconds
	int32 Buttons     = 0;
};

// --------------------------------------------------------------------------
// QMovement — self-contained movement class (replaces pmove global state)
//
// All movement parameters are read from a UMovementSettings* each frame.
// If Settings is nullptr, QuakeWorld defaults are used.
// --------------------------------------------------------------------------
class QMovement
{
public:
	QMovement();

	// ---- Call this every Tick from your APawn ----
	// Settings may be nullptr (uses QuakeWorld defaults then).
	// Owner = the APawn actor (used to ignore self in collision traces).
	void ProcessMovement(float DeltaTime, UWorld* World, const UMovementSettings* Settings, AActor* Owner);

	// =====================================================================
	// Player state (equivalent to playermove_t fields)
	// =====================================================================
	FVector Origin   = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector BaseVelocity = FVector::ZeroVector; // Conveyors, pushes
	FVector ViewAngles = FVector::ZeroVector;  // (Pitch, Yaw, Roll)

	int32 OldButtons   = 0;
	float WaterJumpTime = 0.0f;
	bool  bDead        = false;
	bool  bSpectator   = false;
	bool  bIsCrouching = false;
	int32 JumpCount    = 0;

	// Input for this frame
	FQUserCmd Cmd;

	// =====================================================================
	// State computed per-frame
	// =====================================================================
	bool  bOnGround   = false;
	int32 OnGroundEnt = -1;      // entity index or -1
	int32 WaterLevel  = 0;
	int32 WaterType   = 0;       // CONTENTS_EMPTY = 0

	// Ladder normal cache (stable across entire climb, like Valve's trace-to-center)
	FVector CachedLadderNormal = FVector::ZeroVector;
	bool bHasLadderNormal = false;

	// Touch list (contacts during move)
	static constexpr int32 MAX_TOUCH = 32;
	int32 NumTouch = 0;
	int32 TouchIndex[MAX_TOUCH];

private:
	// =====================================================================
	// Active settings for current frame (copied from UMovementSettings or defaults)
	// =====================================================================
	float Sv_Gravity          = 800.0f;
	float Sv_StopSpeed        = 100.0f;
	float Sv_MaxSpeed         = 320.0f;
	float Sv_SpectatorMaxSpeed= 500.0f;
	float Sv_Accelerate       = 10.0f;
	float Sv_AirAccelerate    = 0.7f;
	float Sv_WaterAccelerate  = 10.0f;
	float Sv_Friction         = 6.0f;
	float Sv_WaterFriction    = 1.0f;
	float Sv_MaxVelocity      = 2000.0f;
	float Sv_EntGravity       = 1.0f;
	float Sv_JumpSpeed        = 270.0f;
	float Sv_StepSize         = 18.0f;
	float Sv_CapsuleRadius    = 16.0f;
	float Sv_CapsuleHalfHeight= 28.0f;
	float Sv_CapsuleHalfHeightCrouch = 18.0f;
	float Sv_AirSpeedCap      = 30.0f;
	float Sv_GroundNormalZ    = 0.7f;
	float Sv_MaxGroundUpVel   = 180.0f;
	bool  Sv_AutoBunnyHop     = false;
	int32 Sv_MaxJumps         = 1;

	// Player mins/maxs (derived from capsule dims)
	FVector PlayerMins = FVector(-16.0f, -16.0f, -28.0f);
	FVector PlayerMaxs = FVector( 16.0f,  16.0f,  28.0f);

	// Loads settings from UMovementSettings (or defaults if null)
	void LoadSettings(const UMovementSettings* Settings);

	// =====================================================================
	// Internal — direct ports of pmove.c functions
	// =====================================================================

	// Frame locals
	float FrameTime = 0.0f;
	FVector Forward, Right, Up;
	UWorld* WorldPtr = nullptr;
	AActor* OwnerActor = nullptr;  // ignored in all traces

	static constexpr int32 BUTTON_JUMP = 2;
	static constexpr int32 BUTTON_DUCK = 4;
	static constexpr float STOP_EPSILON = 0.1f;
	static constexpr int32 MAX_CLIP_PLANES = 5;

	// Movement functions (1:1 port)
	int32 ClipVelocity(const FVector& In, const FVector& Normal, FVector& Out, float Overbounce);
	int32 FlyMove();
	void  GroundMove();
	void  ApplyFriction();
	void  Accelerate(const FVector& WishDir, float WishSpeed, float Accel);
	void  AirAccelerate(const FVector& WishDir, float WishSpeed, float Accel);
	void  AirMove();
	void  WaterMove();
	void  CategorizePosition();
	void  JumpButton();
	void  DuckButton();
	void  CheckWaterJump();
	void  NudgePosition();
	void  SpectatorMove();
	void  CheckVelocity();
	void  ResolveOverlaps();
	void  AddBaseVelocity();

	// Trace helpers — bridge to UE5 collision
	FQMoveTrace PlayerMoveTrace(const FVector& Start, const FVector& End, bool bFindInitialOverlaps = false);
	bool TestPlayerPosition(const FVector& Pos);
	int32 PointContents(const FVector& Point);

	// Angle helpers
	void ComputeViewVectors();
};
