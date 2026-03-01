// QMovement.cpp — GoldSrc movement adapts to UE5 C++
// Version 1.1 | Developed by MrKamkar (https://github.com/MrKamkar)
// Copyright (C) 1996-1997 Id Software, Inc. (GPL v2)

#include "QMovement.h"
#include "UMovementSettings.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"

// --------------------------------------------------------------------------
// Valve's PM_GetRandomStuckOffsets array
// --------------------------------------------------------------------------
const FVector QMovement::rgv3tStuckTable[54] = {
	FVector(0,0,0),    FVector(0,0,1),    FVector(0,0,2),    FVector(0,0,3),    FVector(0,0,4),    FVector(0,0,5),
	FVector(-1,0,0),   FVector(1,0,0),    FVector(0,-1,0),   FVector(0,1,0),    FVector(-1,-1,0),  FVector(1,-1,0),
	FVector(-1,1,0),   FVector(1,1,0),    FVector(-2,0,0),   FVector(2,0,0),    FVector(0,-2,0),   FVector(0,2,0),
	FVector(-2,-2,0),  FVector(2,-2,0),   FVector(-2,2,0),   FVector(2,2,0),    FVector(-3,0,0),   FVector(3,0,0),
	FVector(0,-3,0),   FVector(0,3,0),    FVector(-3,-3,0),  FVector(3,-3,0),   FVector(-3,3,0),   FVector(3,3,0),
	FVector(-4,0,0),   FVector(4,0,0),    FVector(0,-4,0),   FVector(0,4,0),    FVector(-4,-4,0),  FVector(4,-4,0),
	FVector(-4,4,0),   FVector(4,4,0),    FVector(-5,0,0),   FVector(5,0,0),    FVector(0,-5,0),   FVector(0,5,0),
	FVector(-5,-5,0),  FVector(5,-5,0),   FVector(-5,5,0),   FVector(5,5,0),    FVector(-6,0,0),   FVector(6,0,0),
	FVector(0,-6,0),   FVector(0,6,0),    FVector(-6,-6,0),  FVector(6,-6,0),   FVector(-6,6,0),   FVector(6,6,0)
};

int32 QMovement::GetRandomStuckOffsets(FVector& OutOffset)
{
	int32 idx = rgStuckLast++;
	OutOffset = rgv3tStuckTable[idx % 54];
	return (idx % 54);
}


// ============================================================================
// Constructor
// ============================================================================
QMovement::QMovement()
{
	FMemory::Memzero(TouchIndex, sizeof(TouchIndex));
}

// ============================================================================
// LoadSettings — copy UMovementSettings into frame-local Sv_* variables
// ============================================================================
void QMovement::LoadSettings(const UMovementSettings* Settings)
{
	if (Settings)
	{
		Sv_Gravity           = Settings->Gravity;
		Sv_StopSpeed         = Settings->StopSpeed;
		Sv_MaxSpeed          = Settings->MaxSpeed;
		Sv_SpectatorMaxSpeed = Settings->SpectatorMaxSpeed;
		Sv_Accelerate        = Settings->Accelerate;
		Sv_AirAccelerate     = Settings->AirAccelerate;
		Sv_WaterAccelerate   = Settings->WaterAccelerate;
		Sv_Friction          = Settings->Friction;
		Sv_WaterFriction     = Settings->WaterFriction;
		Sv_EntGravity        = Settings->EntGravity;
		Sv_JumpSpeed         = Settings->JumpSpeed;
		Sv_StepSize          = Settings->StepSize;
		Sv_CapsuleRadius     = Settings->CapsuleRadius;
		Sv_CapsuleHalfHeight = Settings->CapsuleHalfHeight;
		Sv_AirSpeedCap       = Settings->AirSpeedCap;
		Sv_GroundNormalZ     = Settings->GroundNormalZ;
		Sv_MaxGroundUpVel    = Settings->MaxGroundUpVelocity;
		Sv_MaxVelocity       = Settings->MaxVelocity;
		Sv_AutoBunnyHop      = Settings->bAutoBunnyHop;
		Sv_DisableBhopCap    = Settings->bDisableBhopCap;
		Sv_EnableCameraPunch = Settings->bEnableCameraPunch;
		Sv_EnableCameraRoll  = Settings->bEnableCameraRoll;
		Sv_EnableBobbing     = Settings->bEnableBobbing;
		Sv_BobAmount         = Settings->BobAmount;
		Sv_BobSpeed          = Settings->BobSpeed;
		Sv_BobUp             = Settings->BobUp;
		Sv_MaxJumps          = Settings->MaxJumps;
		Sv_CapsuleHalfHeightCrouch = Settings->CapsuleHalfHeightCrouch;
		Sv_ViewHeight        = Settings->ViewHeight;
		Sv_ViewHeightCrouch  = Settings->ViewHeightCrouch;
	}

	// Derive player mins/maxs from capsule dimensions
	PlayerMins = FVector(-Sv_CapsuleRadius, -Sv_CapsuleRadius, -Sv_CapsuleHalfHeight);
	PlayerMaxs = FVector( Sv_CapsuleRadius,  Sv_CapsuleRadius,  Sv_CapsuleHalfHeight);
}

// ============================================================================
// ComputeViewVectors — replaces AngleVectors(pmove.angles, forward, right, up)
// ============================================================================
void QMovement::ComputeViewVectors()
{
	const FRotator Rot(ViewAngles.X, ViewAngles.Y, ViewAngles.Z);
	const FRotationMatrix Mat(Rot);
	Forward = Mat.GetUnitAxis(EAxis::X);
	Right   = Mat.GetUnitAxis(EAxis::Y);
	Up      = Mat.GetUnitAxis(EAxis::Z);
}

// ============================================================================
// Trace helpers — bridge to UE5 collision
// ============================================================================

FQMoveTrace QMovement::PlayerMoveTrace(const FVector& Start, const FVector& End, bool bFindInitialOverlaps)
{
	FQMoveTrace Result;

	if (!WorldPtr)
	{
		Result.Fraction = 1.0f;
		Result.EndPos = End;
		return Result;
	}

	FHitResult Hit;
	// Sweep with exact capsule dimensions
	FCollisionShape Shape = FCollisionShape::MakeCapsule(Sv_CapsuleRadius, Sv_CapsuleHalfHeight);
	FCollisionQueryParams Params;
	Params.bReturnPhysicalMaterial = false;
	Params.bFindInitialOverlaps = bFindInitialOverlaps;
	if (OwnerActor) Params.AddIgnoredActor(OwnerActor);

	const bool bHit = WorldPtr->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Shape,
		Params
	);

	if (bHit)
	{
		// Mask UE5 false-positive penetrations when caller asked to ignore initial overlaps
		if (!bFindInitialOverlaps && Hit.Time == 0.0f)
		{
			Result.bStartSolid = false;
			Result.bAllSolid   = false;
		}
		else
		{
			Result.bStartSolid = Hit.bStartPenetrating;
			Result.bAllSolid   = Hit.bStartPenetrating && (Hit.Time <= 0.0f);
		}

		Result.Fraction    = Hit.Time;
		Result.EndPos      = Hit.Time > 0.0f ? Hit.Location : Start;
		Result.PlaneNormal = Hit.ImpactNormal;
		Result.Ent         = Hit.GetActor() ? 1 : 0; // 0 = world, 1+ = entity
	}
	else
	{
		Result.Fraction    = 1.0f;
		Result.EndPos      = End;
		Result.PlaneNormal = FVector::ZeroVector;  // no surface hit
		Result.Ent         = -1;
	}

	return Result;
}

bool QMovement::TestPlayerPosition(const FVector& Pos, float ShrinkScale)
{
	// Returns true if position is VALID (not stuck in solid)
	if (!WorldPtr) return true;

	FCollisionShape Shape = FCollisionShape::MakeCapsule(
		FMath::Max(1.0f, Sv_CapsuleRadius - ShrinkScale), 
		FMath::Max(1.0f, Sv_CapsuleHalfHeight - ShrinkScale)
	);
	FCollisionQueryParams Params;
	if (OwnerActor) Params.AddIgnoredActor(OwnerActor);

	return !WorldPtr->OverlapBlockingTestByChannel(
		Pos,
		FQuat::Identity,
		ECC_Pawn,
		Shape,
		Params
	);
}

int32 QMovement::PointContents(const FVector& Point)
{
	if (!WorldPtr) return 0;

	// Quake: 0=Empty, -1=Solid, -2=Water, -4=Slime, -5=Lava, -10=Ladder (custom)
	
	// Use a small sphere (8 units) for trigger detection.
	// 2.0 was too small and could miss triggers if the origin was slightly offset.
	FCollisionShape Shape = FCollisionShape::MakeSphere(8.0f);
	FCollisionQueryParams Params;
	Params.bReturnPhysicalMaterial = false;
	if (OwnerActor) Params.AddIgnoredActor(OwnerActor);

	TArray<FOverlapResult> Overlaps;
	WorldPtr->OverlapMultiByChannel(
		Overlaps,
		Point,
		FQuat::Identity,
		ECC_Pawn,
		Shape,
		Params
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (HitActor)
		{
			// Check if it's a water volume (either by actor type or tag)
			if (HitActor->ActorHasTag(FName("Water")))
			{
				return -2; // CONTENTS_WATER
			}
			if (HitActor->ActorHasTag(FName("Slime")))
			{
				return -4; // CONTENTS_SLIME
			}
			if (HitActor->ActorHasTag(FName("Lava")))
			{
				return -5; // CONTENTS_LAVA
			}
			if (HitActor->ActorHasTag(FName("Ladder")))
			{
				return -10; // Custom CONTENTS_LADDER
			}
		}
	}

	return 0; // CONTENTS_EMPTY
}

// ============================================================================
// PM_ClipVelocity — Slide off of the impacting object
// Returns blocked flags: 1 = floor, 2 = step/wall
// ============================================================================
int32 QMovement::ClipVelocity(const FVector& In, const FVector& Normal, FVector& Out, float Overbounce)
{
	int32 Blocked = 0;

	if (Normal.Z > 0.0f)
		Blocked |= 1;  // floor
	if (Normal.Z == 0.0f)
		Blocked |= 2;  // step

	const float Backoff = FVector::DotProduct(In, Normal) * Overbounce;

	Out.X = In.X - Normal.X * Backoff;
	Out.Y = In.Y - Normal.Y * Backoff;
	Out.Z = In.Z - Normal.Z * Backoff;

	// Clamp tiny values to zero
	if (Out.X > -STOP_EPSILON && Out.X < STOP_EPSILON) Out.X = 0.0f;
	if (Out.Y > -STOP_EPSILON && Out.Y < STOP_EPSILON) Out.Y = 0.0f;
	if (Out.Z > -STOP_EPSILON && Out.Z < STOP_EPSILON) Out.Z = 0.0f;

	return Blocked;
}

// ============================================================================
// PM_FlyMove — basic solid body movement clip that slides along planes
// ============================================================================
int32 QMovement::FlyMove()
{
	constexpr int32 NumBumps = 4;
	int32 Blocked = 0;

	const FVector OriginalVelocity = Velocity;
	const FVector PrimalVelocity   = Velocity;
	int32 NumPlanes = 0;
	FVector Planes[MAX_CLIP_PLANES];

	float TimeLeft = FrameTime;

	for (int32 BumpCount = 0; BumpCount < NumBumps; BumpCount++)
	{
		const FVector End = Origin + Velocity * TimeLeft;
		FQMoveTrace Trace = PlayerMoveTrace(Origin, End);

		if (Trace.bStartSolid || Trace.bAllSolid)
		{
			Velocity = FVector::ZeroVector;
			return 3;
		}

		if (Trace.Fraction > 0.0f)
		{
			Origin = Trace.EndPos;
			NumPlanes = 0;
		}

		if (Trace.Fraction == 1.0f)
			break;

		// UE5 FIX: Ignore false hits where velocity points away from the plane
		if (FVector::DotProduct(Velocity, Trace.PlaneNormal) > 0.01f)
		{
			Origin += Trace.PlaneNormal * 0.1f;
			continue;
		}

		// Save entity for contact
		if (NumTouch < MAX_TOUCH)
		{
			TouchIndex[NumTouch] = Trace.Ent;
			NumTouch++;
		}

		if (Trace.PlaneNormal.Z > Sv_GroundNormalZ)
			Blocked |= 1;  // floor
		if (Trace.PlaneNormal.Z == 0.0f)
			Blocked |= 2;  // step

		TimeLeft -= TimeLeft * Trace.Fraction;

		// Prevent infinite zero-fraction snag loops!
		// Unreal will sometimes repeatedly return Time=0 against the exact same floor or wall when sliding.
		bool bDuplicatePlane = false;
		for (int32 k = 0; k < NumPlanes; k++)
		{
			if (Planes[k].Equals(Trace.PlaneNormal, 0.01f))
			{
				bDuplicatePlane = true;
				break;
			}
		}

		// Duplicate plane snag — micro-nudge to escape (Quake DIST_EPSILON)
		if (bDuplicatePlane && Trace.Fraction == 0.0f)
		{
			Origin += Trace.PlaneNormal * 0.0625f;
			Velocity += Trace.PlaneNormal * 0.0625f;
			continue;
		}

		if (NumPlanes >= MAX_CLIP_PLANES)
		{
			Velocity = FVector::ZeroVector;
			break;
		}

		Planes[NumPlanes] = Trace.PlaneNormal;
		NumPlanes++;

		// Modify original_velocity so it parallels all of the clip planes
		int32 i, j;
		for (i = 0; i < NumPlanes; i++)
		{
			ClipVelocity(OriginalVelocity, Planes[i], Velocity, 1.0f);
			for (j = 0; j < NumPlanes; j++)
			{
				if (j != i)
				{
					if (FVector::DotProduct(Velocity, Planes[j]) < 0.0f)
						break;
				}
			}
			if (j == NumPlanes)
				break;
		}

		if (i != NumPlanes)
		{
			// Go along this plane
		}
		else
		{
			// Go along the crease
			if (NumPlanes != 2)
			{
				Velocity = FVector::ZeroVector;
				break;
			}
			const FVector Dir = FVector::CrossProduct(Planes[0], Planes[1]);
			const float D = FVector::DotProduct(Dir, Velocity);
			Velocity = Dir * D;
		}

		// If velocity reversed against original, stop to avoid oscillation
		if (FVector::DotProduct(Velocity, PrimalVelocity) <= 0.0f)
		{
			break;
		}
	}

	if (WaterJumpTime > 0.0f)
	{
		Velocity = PrimalVelocity;
	}

	return Blocked;
}

// ============================================================================
// PM_WalkMove — Player is on ground, with no upwards velocity
// ============================================================================
void QMovement::WalkMove()
{
	Velocity.Z = 0.0f;
	if (Velocity.IsNearlyZero())
		return;

	// First try just moving to the destination
	FVector Dest;
	Dest.X = Origin.X + Velocity.X * FrameTime;
	Dest.Y = Origin.Y + Velocity.Y * FrameTime;
	Dest.Z = Origin.Z;

	FQMoveTrace Trace = PlayerMoveTrace(Origin, Dest);
	if (Trace.Fraction == 1.0f)
	{
		Origin = Trace.EndPos;
		return;
	}

	// Try sliding forward both on ground and up StepSize pixels
	const FVector OriginalPos = Origin;
	const FVector OriginalVel = Velocity;

	// Slide move (down path)
	FlyMove();
	const FVector DownPos = Origin;
	const FVector DownVel = Velocity;

	// Reset
	Origin   = OriginalPos;
	Velocity = OriginalVel;

	// Move up a stair height
	Dest = Origin;
	Dest.Z += Sv_StepSize;
	Trace = PlayerMoveTrace(Origin, Dest);
	if (!Trace.bStartSolid && !Trace.bAllSolid)
	{
		Origin = Trace.EndPos;
	}

	// Slide move (up path)
	FlyMove();

	// Press down the step height
	Dest = Origin;
	Dest.Z -= Sv_StepSize;
	Trace = PlayerMoveTrace(Origin, Dest);
	if (Trace.PlaneNormal.Z < Sv_GroundNormalZ)
		goto usedown;
	if (!Trace.bStartSolid && !Trace.bAllSolid)
	{
		Origin = Trace.EndPos;
	}

	{
		const FVector UpPos = Origin;

		const float DownDist = FMath::Square(DownPos.X - OriginalPos.X) + FMath::Square(DownPos.Y - OriginalPos.Y);
		const float UpDist   = FMath::Square(UpPos.X   - OriginalPos.X) + FMath::Square(UpPos.Y   - OriginalPos.Y);

		if (DownDist > UpDist)
		{
			goto usedown;
		}
		else
		{
			Velocity.Z = DownVel.Z;
			return;
		}
	}

usedown:
	Origin   = DownPos;
	Velocity = DownVel;
}

// ============================================================================
// PM_Friction — Handles both ground friction and water friction
// ============================================================================
void QMovement::Friction()
{
	if (WaterJumpTime > 0.0f || WaterLevel == 4)
		return;

	const float Speed = Velocity.Size();
	if (Speed < 1.0f)
	{
		Velocity.X = 0.0f;
		Velocity.Y = 0.0f;
		return;
	}

	float Friction = Sv_Friction;

	// If the leading edge is over a dropoff, increase friction
	if (bOnGround)
	{
		FVector Start2, Stop2;
		Start2.X = Stop2.X = Origin.X + Velocity.X / Speed * 16.0f;
		Start2.Y = Stop2.Y = Origin.Y + Velocity.Y / Speed * 16.0f;
		Start2.Z = Origin.Z + PlayerMins.Z;
		Stop2.Z  = Start2.Z - 34.0f;

		FQMoveTrace Trace = PlayerMoveTrace(Start2, Stop2);
		if (Trace.Fraction == 1.0f)
		{
			Friction *= 2.0f;
		}
	}

	float Drop = 0.0f;

	if (WaterLevel >= 2)
	{
		Drop += Speed * Sv_WaterFriction * (float)WaterLevel * FrameTime;
	}
	else if (bOnGround)
	{
		const float Control = (Speed < Sv_StopSpeed) ? Sv_StopSpeed : Speed;
		Drop += Control * Friction * FrameTime;
	}

	float NewSpeed = Speed - Drop;
	if (NewSpeed < 0.0f)
		NewSpeed = 0.0f;
	NewSpeed /= Speed;

	Velocity *= NewSpeed;
}

// ============================================================================
// PM_Accelerate
// ============================================================================
void QMovement::Accelerate(const FVector& WishDir, float WishSpeed, float Accel)
{
	if (bDead)      return;
	if (WaterJumpTime > 0.0f) return;

	const float CurrentSpeed = FVector::DotProduct(Velocity, WishDir);
	const float AddSpeed = WishSpeed - CurrentSpeed;
	if (AddSpeed <= 0.0f)
		return;

	float AccelSpeed = Accel * FrameTime * WishSpeed;
	if (AccelSpeed > AddSpeed)
		AccelSpeed = AddSpeed;

	Velocity += AccelSpeed * WishDir;
}

// ============================================================================
// PM_AirAccelerate — the magic behind air-strafing and bhop
// ============================================================================
void QMovement::AirAccelerate(const FVector& WishDir, float WishSpeed, float Accel)
{
	if (bDead)      return;
	if (WaterJumpTime > 0.0f) return;

	float WishSpd = WishSpeed;
	if (WishSpd > Sv_AirSpeedCap)
		WishSpd = Sv_AirSpeedCap;

	const float CurrentSpeed = FVector::DotProduct(Velocity, WishDir);
	const float AddSpeed = WishSpd - CurrentSpeed;
	if (AddSpeed <= 0.0f)
		return;

	float AccelSpeed = Accel * WishSpeed * FrameTime;
	if (AccelSpeed > AddSpeed)
		AccelSpeed = AddSpeed;

	Velocity += AccelSpeed * WishDir;
}

// ============================================================================
// PM_AirMove — main ground/air movement dispatch
// ============================================================================
void QMovement::AirMove()
{
	const float FMove = Cmd.ForwardMove;
	const float SMove = Cmd.SideMove;

	FVector Fwd = Forward;
	FVector Rht = Right;
	Fwd.Z = 0.0f;
	Rht.Z = 0.0f;
	Fwd.Normalize();
	Rht.Normalize();

	FVector WishVel;
	WishVel.X = Fwd.X * FMove + Rht.X * SMove;
	WishVel.Y = Fwd.Y * FMove + Rht.Y * SMove;
	WishVel.Z = 0.0f;

	FVector WishDir = WishVel;
	float WishSpeed = WishDir.Size();
	if (WishSpeed > SMALL_NUMBER)
		WishDir /= WishSpeed;
	else
		WishDir = FVector::ZeroVector;

	// Clamp to server defined max speed
	if (WishSpeed > Sv_MaxSpeed)
	{
		WishVel *= Sv_MaxSpeed / WishSpeed;
		WishSpeed = Sv_MaxSpeed;
	}

	if (bOnGround && WaterJumpTime == 0.0f)
	{
		Velocity.Z = 0.0f;
		Accelerate(WishDir, WishSpeed, Sv_Accelerate);
		Velocity.Z = 0.0f;
		
		WalkMove();
	}
	else
	{
		// Air path (bhop / air-strafe / surf / water jump)
		if (WaterJumpTime == 0.0f)
		{
			AirAccelerate(WishDir, WishSpeed, Sv_AirAccelerate);
		}

		FlyMove();
	}
}

// ============================================================================
// PM_WaterMove
// ============================================================================
void QMovement::WaterMove()
{
	FVector WishVel;
	for (int32 i = 0; i < 3; i++)
	{
		WishVel[i] = Forward[i] * Cmd.ForwardMove + Right[i] * Cmd.SideMove;
	}

	if (Cmd.ForwardMove == 0.0f && Cmd.SideMove == 0.0f && Cmd.UpMove == 0.0f)
	{
		WishVel.Z -= 60.0f;
	}
	else
	{
		WishVel.Z += Cmd.UpMove;
	}

	FVector WishDir = WishVel;
	float WishSpeed = WishDir.Size();
	if (WishSpeed > SMALL_NUMBER)
		WishDir /= WishSpeed;
	else
		WishDir = FVector::ZeroVector;

	if (WishSpeed > Sv_MaxSpeed)
	{
		WishVel *= Sv_MaxSpeed / WishSpeed;
		WishSpeed = Sv_MaxSpeed;
	}
	WishSpeed *= 0.7f;

	Accelerate(WishDir, WishSpeed, Sv_WaterAccelerate);

	FVector Dest = Origin + Velocity * FrameTime;
	FVector Start2 = Dest;
	Start2.Z += Sv_StepSize + 1.0f;
	FQMoveTrace Trace = PlayerMoveTrace(Start2, Dest);
	if (!Trace.bStartSolid && !Trace.bAllSolid)
	{
		Origin = Trace.EndPos;
		return;
	}

	FlyMove();
}

// ============================================================================
// PM_CatagorizePosition — determine onground, waterlevel, watertype
//
// Order: ladder → water → ground trace.
// Ladder and water use cheap PointContents overlaps. The expensive capsule
// sweep ground trace is SKIPPED entirely when in water or on a ladder,
// avoiding wasted traces and the floor-snap problem.
// ============================================================================
void QMovement::CategorizePosition()
{
	// ==================================================================
	// Phase 1: Ladder detection (cheap sphere overlaps × up to 3)
	// ==================================================================
	WaterLevel = 0;
	WaterType  = 0; // CONTENTS_EMPTY

	FVector CheckPoint = Origin;
	bool bFoundLadder = false;

	// Foot level (4 units up from soles, to be robust against floor-snap offsets)
	CheckPoint.Z = Origin.Z + PlayerMins.Z + 4.0f;
	if (PointContents(CheckPoint) == -10) bFoundLadder = true;

	// Waist level
	if (!bFoundLadder)
	{
		CheckPoint.Z = Origin.Z;
		if (PointContents(CheckPoint) == -10) bFoundLadder = true;
	}

	// Eye level
	if (!bFoundLadder)
	{
		CheckPoint.Z = Origin.Z + 22.0f;
		if (PointContents(CheckPoint) == -10) bFoundLadder = true;
	}

	if (bFoundLadder)
	{
		WaterType   = -10;
		WaterLevel  = 4;
		bOnGround   = false;
		OnGroundEnt = -1;
		return; // On ladder — skip ground trace entirely
	}

	// ==================================================================
	// Phase 2: Water detection (cheap sphere overlaps × up to 3)
	// ==================================================================
	CheckPoint.X = Origin.X;
	CheckPoint.Y = Origin.Y;
	CheckPoint.Z = Origin.Z + PlayerMins.Z + 4.0f;
	int32 Cont = PointContents(CheckPoint);

	if (Cont <= -2) // <= CONTENTS_WATER
	{
		WaterType  = Cont;
		WaterLevel = 1;
		bOnGround   = false;
		OnGroundEnt = -1;

		// Check waist level
		CheckPoint.Z = Origin.Z + (PlayerMins.Z + PlayerMaxs.Z) * 0.5f;
		Cont = PointContents(CheckPoint);
		if (Cont <= -2)
		{
			WaterLevel = 2;

			// Check eye level
			CheckPoint.Z = Origin.Z + 22.0f;
			Cont = PointContents(CheckPoint);
			if (Cont <= -2)
			{
				WaterLevel = 3; // Fully submerged
			}
		}
		return; // In water — skip ground trace (no floor snap)
	}

	// ==================================================================
	// Phase 3: Ground trace (capsule sweep — only on dry land)
	// ==================================================================
	if (Velocity.Z > Sv_MaxGroundUpVel)
	{
		bOnGround   = false;
		OnGroundEnt = -1;
		return;
	}

	// Single sweep: gets ground normal, fraction, AND actor in one call
	FVector Point = Origin;
	Point.Z -= 2.0f;

	FCollisionShape Shape = FCollisionShape::MakeCapsule(Sv_CapsuleRadius, Sv_CapsuleHalfHeight);
	FCollisionQueryParams Params;
	Params.bFindInitialOverlaps = true;
	if (OwnerActor) Params.AddIgnoredActor(OwnerActor);

	FHitResult Hit;
	const bool bHit = WorldPtr->SweepSingleByChannel(
		Hit, Origin, Point, FQuat::Identity, ECC_Pawn, Shape, Params
	);

	if (!bHit || Hit.ImpactNormal.Z < Sv_GroundNormalZ)
	{
		bOnGround   = false;
		OnGroundEnt = -1;
	}
	else
	{
		if (!bOnGround)
		{
			// Player just landed
			bOnGround = true;
			CheckFalling();
		}

		OnGroundEnt = Hit.GetActor() ? 1 : 0;

		AActor* HitActor = Hit.GetActor();

		JumpCount = 0;
		WaterJumpTime = 0.0f;

		// Extract velocity from moving platform (lifts, conveyors)
		if (HitActor && OnGroundEnt > 0)
		{
			BaseVelocity = HitActor->GetVelocity();
		}

		if (!Hit.bStartPenetrating)
		{
			Origin = Hit.Location;
		}
	}

	// Touch list
	if (bHit && Hit.GetActor() && NumTouch < MAX_TOUCH)
	{
		TouchIndex[NumTouch] = 1;
		NumTouch++;
	}
}

// ============================================================================
// PreventMegaBunnyJumping
// ============================================================================
void QMovement::PreventMegaBunnyJumping()
{
	if (Sv_DisableBhopCap)
		return;

	constexpr float BUNNYJUMP_MAX_SPEED_FACTOR = 1.7f;
	float MaxScaledSpeed = BUNNYJUMP_MAX_SPEED_FACTOR * Sv_MaxSpeed;

	if (MaxScaledSpeed <= 0.0f)
		return;

	float Spd = Velocity.Size();

	if (Spd <= MaxScaledSpeed)
		return;

	float Fraction = (MaxScaledSpeed / Spd) * 0.65f;
	Velocity *= Fraction;
}

// ============================================================================
// PM_Jump
// ============================================================================
void QMovement::Jump()
{
	if (bDead)
	{
		OldButtons |= BUTTON_JUMP;
		return;
	}

	if (WaterJumpTime > 0.0f)
	{
		return;
	}

	if (WaterLevel >= 2)
	{
		bOnGround   = false;
		OnGroundEnt = -1;
		
		int32 Cont = PointContents(Origin);
		if (Cont == -4) // CONTENTS_SLIME
			Velocity.Z = 80.0f;
		else if (Cont == -5) // CONTENTS_LAVA
			Velocity.Z = 50.0f;
		else
			Velocity.Z = 100.0f;
			
		return;
	}

	// Double-jump logic: check JumpCount against Sv_MaxJumps
	if (!bOnGround && JumpCount >= Sv_MaxJumps)
		return;

	// In the air: Only allow jumping again if we RELEASED the jump button first (no auto-bhop double jump)
	if (!bOnGround && (OldButtons & BUTTON_JUMP))
		return;

	// On the ground: Check autobhop preference
	if (bOnGround && !Sv_AutoBunnyHop && (OldButtons & BUTTON_JUMP))
		return;  // don't pogo stick

	bOnGround   = false;
	OnGroundEnt = -1;

	// Inherit base velocity to gain momentum from conveyors/lifts when jumping!
	Velocity += BaseVelocity;

	PreventMegaBunnyJumping();

	// Perform the jump natively
	Velocity.Z = Sv_JumpSpeed;

	// Decay it for simulation
	FixupGravityVelocity();

	JumpCount++;
	OldButtons |= BUTTON_JUMP;
}

// ============================================================================
// UnDuck
// ============================================================================
void QMovement::UnDuck()
{
	const float Diff = PlayerMaxs.Z - Sv_CapsuleHalfHeightCrouch;
	
	// Case 1: On ground (or near ground). Try shifting head UP.
	FVector NewOrigin = Origin;
	NewOrigin.Z += Diff;

	Sv_CapsuleHalfHeight = PlayerMaxs.Z;
	FQMoveTrace Trace = PlayerMoveTrace(NewOrigin, NewOrigin);

	if (!Trace.bStartSolid && !Trace.bAllSolid)
	{
		bIsCrouching = false;
		bInDuck = false;
		flDuckTime = 0;
		Origin = NewOrigin;
		ViewOffset.Z = 22.0f;
		CategorizePosition();
		return;
	}

	// Case 2: In air. Try expanding from center (Leg tuck release).
	// This matches GoldSrc PM_UnDuck logic where origin doesn't shift in air.
	NewOrigin = Origin;
	Trace = PlayerMoveTrace(NewOrigin, NewOrigin);

	if (!Trace.bStartSolid && !Trace.bAllSolid)
	{
		bIsCrouching = false;
		bInDuck = false;
		flDuckTime = 0;
		Origin = NewOrigin;
		ViewOffset.Z = Sv_ViewHeight;
		CategorizePosition();
		return;
	}

	// Still stuck? Stay crouched.
	Sv_CapsuleHalfHeight = Sv_CapsuleHalfHeightCrouch;
	ViewOffset.Z = Sv_ViewHeightCrouch;
}

void QMovement::Duck()
{
	int32 ButtonsChanged = (OldButtons ^ Cmd.Buttons);
	int32 ButtonPressed = ButtonsChanged & Cmd.Buttons;

	if (flDuckTime > 0)
	{
		flDuckTime -= (int32)(FrameTime * 1000.0f);
		if (flDuckTime < 0) flDuckTime = 0;
	}

	if (bDead)
	{
		if (bIsCrouching)
		{
			UnDuck();
		}
		return;
	}

	if (bIsCrouching)
	{
		Cmd.ForwardMove *= PLAYER_DUCKING_MULTIPLIER;
		Cmd.SideMove    *= PLAYER_DUCKING_MULTIPLIER;
		Cmd.UpMove      *= PLAYER_DUCKING_MULTIPLIER;
	}

	if ((Cmd.Buttons & BUTTON_DUCK) || bInDuck || bIsCrouching)
	{
		if (Cmd.Buttons & BUTTON_DUCK)
		{
			// START DUCKING
			if ((ButtonPressed & BUTTON_DUCK) && !bIsCrouching && !bInDuck)
			{
				flDuckTime = 1000;
				bInDuck = true;
			}

			float Time = FMath::Max(0.0f, (1.0f - (float)flDuckTime / 1000.0f));

			if (bInDuck)
			{
				const float Diff = PlayerMaxs.Z - Sv_CapsuleHalfHeightCrouch;

				// Finish ducking immediately if in air, or after TIME_TO_DUCK on ground
				if (!bOnGround || (Time >= TIME_TO_DUCK))
				{
					bInDuck = false;
					bIsCrouching = true;
					ViewOffset.Z = Sv_ViewHeightCrouch;
					
					// Physics shift (hull and origin) happens NOW
					Sv_CapsuleHalfHeight = Sv_CapsuleHalfHeightCrouch;
					if (bOnGround)
					{
						Origin.Z -= Diff;
						CategorizePosition();
					}
				}
				else
				{
					// Spline view down while still in stand hull
					float DuckFraction = SplineFraction(Time, (1.0f / TIME_TO_DUCK));
					// Goes from StandView down to CrouchView-Relative-To-StandOrigin
					ViewOffset.Z = (Sv_ViewHeight * (1.0f - DuckFraction)) + ((Sv_ViewHeightCrouch - Diff) * DuckFraction);
				}
			}
			else if (bIsCrouching)
			{
				ViewOffset.Z = Sv_ViewHeightCrouch;
			}
		}
		else
		{
			// TRY TO UNDUCK
			UnDuck();
		}
	}
}

// ============================================================================
// CheckWaterJump
// ============================================================================
void QMovement::CheckWaterJump()
{
	if (WaterJumpTime > 0.0f)
		return;

	if (Velocity.Z < -Sv_MaxGroundUpVel)
		return;

	// Require player to be actively trying to move forward
	if (Cmd.ForwardMove <= 0.0f)
		return;

	FVector FlatForward = Forward;
	FlatForward.Z = 0.0f;
	FlatForward.Normalize();

	FVector Spot = Origin + 24.0f * FlatForward;
	Spot.Z += 8.0f;
	int32 Cont = PointContents(Spot);
	if (Cont != -2 && Cont != -4 && Cont != -5) // Water, Slime, or Lava
		return;

	Spot.Z += 24.0f;
	Cont = PointContents(Spot);
	if (Cont != 0)
		return;

	Velocity = FlatForward * 50.0f;
	Velocity.Z = 225.0f; // Authentic GoldSrc Z velocity for water jump
	WaterJumpTime = 2.0f;
	OldButtons |= BUTTON_JUMP;
}

// ============================================================================
// NudgePosition — if in solid, try nudging to get unstuck
// ============================================================================
void QMovement::NudgePosition()
{
	// Only try to unstick if actually stuck
	// We shrink the capsule by 0.5f to ensure flush wall contacts (surfing) 
	// are NOT misidentified as "stuck" by the overlap sweep!
	if (TestPlayerPosition(Origin, 0.5f))
	{
		// Position is valid, we are not stuck. Skip nudging.
		return;
	}

	const FVector Base = Origin;

	for (int32 i = 0; i < 3; i++)
	{
		Origin[i] = FMath::FloorToFloat(Origin[i] * 8.0f) * 0.125f;
	}

	FVector TestOffset;

	for (int32 i = 0; i < 54; i++)
	{
		GetRandomStuckOffsets(TestOffset);
		
		Origin = Base + TestOffset;
		if (TestPlayerPosition(Origin, 0.5f))
			return; // Found a VALID (non-solid) position!
	}

	// If we failed to find an offset, revert and try old basic tests if absolutely necessary, 
	// but Valve just reverts to base
	Origin = Base;
}

// ============================================================================
// SpectatorMove
// ============================================================================
void QMovement::SpectatorMove()
{
	float Speed = Velocity.Size();
	if (Speed < 1.0f)
	{
		Velocity = FVector::ZeroVector;
	}
	else
	{
		float Drop = 0.0f;
		float Friction = Sv_Friction * 1.5f;
		float Control = (Speed < Sv_StopSpeed) ? Sv_StopSpeed : Speed;
		Drop += Control * Friction * FrameTime;

		float NewSpeed = Speed - Drop;
		if (NewSpeed < 0.0f)
			NewSpeed = 0.0f;
		NewSpeed /= Speed;

		Velocity *= NewSpeed;
	}

	const float FMove = Cmd.ForwardMove;
	const float SMove = Cmd.SideMove;

	FVector Fwd = Forward; Fwd.Normalize();
	FVector Rht = Right;   Rht.Normalize();

	FVector WishVel;
	for (int32 i = 0; i < 3; i++)
		WishVel[i] = Fwd[i] * FMove + Rht[i] * SMove;
	WishVel.Z += Cmd.UpMove;

	FVector WishDir = WishVel;
	float WishSpeed = WishDir.Size();
	if (WishSpeed > SMALL_NUMBER)
		WishDir /= WishSpeed;
	else
		WishDir = FVector::ZeroVector;

	if (WishSpeed > Sv_SpectatorMaxSpeed)
	{
		WishVel *= Sv_SpectatorMaxSpeed / WishSpeed;
		WishSpeed = Sv_SpectatorMaxSpeed;
	}

	const float CurrentSpeed = FVector::DotProduct(Velocity, WishDir);
	const float AddSpeed = WishSpeed - CurrentSpeed;
	if (AddSpeed <= 0.0f)
		return;

	float AccelSpeed = Sv_Accelerate * FrameTime * WishSpeed;
	if (AccelSpeed > AddSpeed)
		AccelSpeed = AddSpeed;

	Velocity += AccelSpeed * WishDir;

	Origin += Velocity * FrameTime;
}

// ============================================================================
// PM_CheckVelocity — clamp velocity to sv_maxvelocity (prevents infinite speed)
// ============================================================================
void QMovement::CheckVelocity()
{
	for (int32 i = 0; i < 3; i++)
	{
		if (Velocity[i] > Sv_MaxVelocity)
			Velocity[i] = Sv_MaxVelocity;
		else if (Velocity[i] < -Sv_MaxVelocity)
			Velocity[i] = -Sv_MaxVelocity;
	}
}

// ============================================================================
// ResolveOverlaps — GoldSrc SV_Push equivalent
// If a movable blocking actor (platform, door) overlaps the player capsule,
// push the player out. Uses OverlapMulti to find ALL overlaps, filters to
// movable objects, then sweeps outward to get UE5 depenetration data.
// ============================================================================
void QMovement::ResolveOverlaps()
{
	if (!WorldPtr || !OwnerActor) return;

	FCollisionShape Shape = FCollisionShape::MakeCapsule(Sv_CapsuleRadius, Sv_CapsuleHalfHeight);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerActor);

	for (int32 Iter = 0; Iter < 8; ++Iter)
	{
		// Find ALL overlapping objects (not just the first one like SweepSingle)
		TArray<FOverlapResult> Overlaps;
		if (!WorldPtr->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Shape, Params))
			break;

		bool bPushed = false;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			UPrimitiveComponent* Comp = Overlap.GetComponent();
			if (!Comp) continue;
			if (Comp->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block) continue;
			if (Comp->Mobility == EComponentMobility::Static) continue;

			// Targeted depenetration from JUST this component (avoids hitting floor/walls)
			FMTDResult MTD;
			if (Comp->ComputePenetration(MTD, Shape, Origin, FQuat::Identity))
			{
				AActor* HitActor = Comp->GetOwner();
				
				// GoldSrc Fix: Do not push out of other players.
				// This prevents players from "repelling" each other like magnets.
				// They will instead block each other solidly via SweepSingle.
				if (HitActor && HitActor->ActorHasTag(FName("Player")))
				{
					continue;
				}

				// Do not push out of water or ladders (even if they are misconfigured to block)
				if (HitActor && (HitActor->ActorHasTag("Water") || HitActor->ActorHasTag("Ladder") || 
								 HitActor->ActorHasTag("Slime") || HitActor->ActorHasTag("Lava")))
				{
					continue;
				}

				// Only push if penetration is significant to avoid jitter on resting contact
				if (MTD.Distance > 0.01f)
				{
					// Use a tiny safety margin (0.01) instead of 0.125 to avoid breaking foot-level triggers
					Origin += MTD.Direction * (MTD.Distance + 0.01f);
					bPushed = true;
				}
			}
			
			if (bPushed) break; // Re-check all overlaps from new position
		}

		if (!bPushed) break;
	}
}

// ============================================================================
// AddBaseVelocity
// ============================================================================
void QMovement::AddBaseVelocity()
{
	// In Half-Life, BaseVelocity is physically applied to the origin 
	// independently of the player's intentional velocity so friction doesn't kill it!
	Origin += BaseVelocity * FrameTime;
}

// ============================================================================
// AddCorrectGravity
// ============================================================================
void QMovement::AddCorrectGravity()
{
	if (WaterJumpTime > 0.0f)
		return;

	Velocity.Z -= (Sv_EntGravity * Sv_Gravity * 0.5f * FrameTime);
	
	CheckVelocity();
}

// ============================================================================
// FixupGravityVelocity
// ============================================================================
void QMovement::FixupGravityVelocity()
{
	if (WaterJumpTime > 0.0f)
		return;

	Velocity.Z -= (Sv_EntGravity * Sv_Gravity * FrameTime * 0.5f);
	CheckVelocity();
}

// ============================================================================
// ProcessMovement — main entry point, replaces PlayerMove()
// ============================================================================
void QMovement::ProcessMovement(float DeltaTime, UWorld* World, const UMovementSettings* Settings, AActor* Owner)
{
	WorldPtr   = World;
	OwnerActor = Owner;
	FrameTime  = DeltaTime;
	NumTouch   = 0;

	// Apply platform velocity from PREVIOUS frame BEFORE ground trace.
	// This moves the player with the platform so CategorizePosition's
	// ground trace finds the platform at its new position.
	AddBaseVelocity();
	BaseVelocity = FVector::ZeroVector;

	// Load all parameters from the Data Asset (or use CS 1.6 defaults)
	LoadSettings(Settings);

	// Duck state forces half height
	if (bIsCrouching)
	{
		Sv_CapsuleHalfHeight = Sv_CapsuleHalfHeightCrouch;
		PlayerMins.Z = -Sv_CapsuleHalfHeight;
	}
	else
	{
		Sv_CapsuleHalfHeight = PlayerMaxs.Z;
		PlayerMins.Z = -Sv_CapsuleHalfHeight;
		ViewOffset.Z = Sv_ViewHeight; // VEC_VIEW
	}

	ComputeViewVectors();

	if (bSpectator)
	{
		SpectatorMove();
		return;
	}

	// Push player out of movable geometry (platforms, doors) — GoldSrc SV_Push
	ResolveOverlaps();

	NudgePosition();

	// Set onground, watertype, and waterlevel
	CategorizePosition();

	if (!bOnGround)
	{
		FallVelocity = -Velocity.Z;
	}

	if (WaterLevel == 2)
		CheckWaterJump();

	if (Velocity.Z < 0.0f)
		WaterJumpTime = 0.0f;

	if (WaterJumpTime > 0.0f)
	{
		WaterJumpTime -= FrameTime;
		if (WaterJumpTime < 0.0f)
			WaterJumpTime = 0.0f;
	}

	if (WaterLevel < 2)
	{
		AddCorrectGravity();
	}

	Duck();

	if (Cmd.Buttons & BUTTON_JUMP)
		Jump();
	else
		OldButtons &= ~BUTTON_JUMP;

	CheckVelocity();

	Friction();

	if (WaterLevel == 4)
	{
		// ==========================================================
		// Authentic PM_LadderMove from Half-Life 1 (pm_shared.c)
		// With cached ladder normal for stability (like Valve's trace-to-center)
		// ==========================================================

		// Check if player's feet are on solid ground
		FVector FloorCheck = Origin;
		FloorCheck.Z += PlayerMins.Z - 1.0f;
		const bool bOnFloor = !TestPlayerPosition(FloorCheck);

		// Get or cache the ladder wall normal
		if (!bHasLadderNormal)
		{
			FVector FlatFwd = Forward; FlatFwd.Z = 0.0f; FlatFwd.Normalize();
			FVector FlatRgt = Right;   FlatRgt.Z = 0.0f; FlatRgt.Normalize();
			
			FVector Dirs[4] = { FlatFwd, -FlatFwd, FlatRgt, -FlatRgt };
			float ClosestDist = 9999.0f;
			FVector BestNormal = -FlatFwd; // fallback
			bool bAnyWall = false;

			FCollisionQueryParams WallParams;
			if (OwnerActor) WallParams.AddIgnoredActor(OwnerActor);

			for (int32 i = 0; i < 4; i++)
			{
				FHitResult WallHit;
				const bool bTrace = WorldPtr->LineTraceSingleByChannel(
					WallHit, Origin, Origin + Dirs[i] * 64.0f, ECC_Pawn, WallParams
				);

				if (bTrace && WallHit.ImpactNormal.Z < 0.5f)
				{
					if (WallHit.Distance < ClosestDist)
					{
						ClosestDist = WallHit.Distance;
						BestNormal = WallHit.ImpactNormal;
						bAnyWall = true;
					}
				}
			}

			CachedLadderNormal = BestNormal;
			bHasLadderNormal = true;
		}

		const FVector& LadderNormal = CachedLadderNormal;
		constexpr float MAX_CLIMB_SPEED = 200.0f;

		float ClimbSpeed = MAX_CLIMB_SPEED;
		if (ClimbSpeed > Sv_MaxSpeed) ClimbSpeed = Sv_MaxSpeed;
		if (bIsCrouching) ClimbSpeed *= 0.333f;

		float FwdAmount = 0.0f, RightAmount = 0.0f;
		if (Cmd.ForwardMove > 0.0f) FwdAmount += ClimbSpeed;
		if (Cmd.ForwardMove < 0.0f) FwdAmount -= ClimbSpeed;
		if (Cmd.SideMove > 0.0f)    RightAmount += ClimbSpeed;
		if (Cmd.SideMove < 0.0f)    RightAmount -= ClimbSpeed;

		if (Cmd.Buttons & BUTTON_JUMP)
		{
			// Jump off the ladder
			Velocity = LadderNormal * 270.0f;
			WaterLevel = 0;
			bHasLadderNormal = false;
		}
		else if (FwdAmount != 0.0f || RightAmount != 0.0f)
		{
			// Valve's velocity decomposition
			FVector IntendedVel = Forward * FwdAmount + Right * RightAmount;

			const FVector WorldUp(0.0f, 0.0f, 1.0f);
			FVector Perp = FVector::CrossProduct(WorldUp, LadderNormal);
			Perp.Normalize();

			const float NormalComponent = FVector::DotProduct(IntendedVel, LadderNormal);
			const FVector Lateral = IntendedVel - LadderNormal * NormalComponent;
			const FVector ClimbDir = FVector::CrossProduct(LadderNormal, Perp);
			Velocity = Lateral - ClimbDir * NormalComponent;

			// onFloor boost: push away from ground when walking into ladder
			if (bOnFloor && NormalComponent > 0.0f)
			{
				Velocity += LadderNormal * MAX_CLIMB_SPEED;
			}
		}
		else
		{
			Velocity = FVector::ZeroVector;
		}

		// UE5-specific: nudge player off the floor so FlyMove can move upward.
		// Valve's MOVETYPE_FLY bypassed floor collision internally; UE5 sweeps cannot.
		if (bOnFloor && Velocity.Z > 0.0f)
		{
			Origin.Z += 2.0f;
			bOnGround = false;
		}

		FlyMove();
	}
	else
	{
		// Not on a ladder — clear cache
		bHasLadderNormal = false;

		if (WaterLevel >= 2)
			WaterMove();
		else
			// Regular movement (walk or fly)
			AirMove();
	}

	CheckVelocity();

	DropPunchAngle();

	if (WaterJumpTime > 0.0f)
	{
		// While water jumping, Valve handles gravity directly in PM_AirMove
		// Since we manage gravity globally outside AirMove, we simulate it here explicitly
		Velocity.Z -= Sv_EntGravity * Sv_Gravity * FrameTime;
	}
	else if (WaterLevel < 2)
	{
		FixupGravityVelocity();
	}

	// Set onground, watertype, and waterlevel for final spot
	// Preserve ladder state: if we were on a ladder, don't let the second
	// CategorizePosition() reset WaterLevel to 0 when the player is still at the base.
	{
		const int32 PrevWaterLevel = WaterLevel;
		const int32 PrevWaterType  = WaterType;
		CategorizePosition();
		// If we were on a ladder this frame, keep it active regardless of floor re-detection
		if (PrevWaterLevel == 4 && WaterLevel != 4)
		{
			WaterLevel = 4;
			WaterType  = PrevWaterType;
			bOnGround  = false;
		}
	}

	// Calculate View Roll
	// Assuming 0.65 as default rollangle and 300.0 as default rollspeed based on HL/CS
	CalcRoll(ViewAngles, Velocity, 0.65f, 300.0f);

	// Calculate View Bobbing
	if (Sv_EnableBobbing && bOnGround)
	{
		float HorizontalSpeed = FMath::Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);
		if (HorizontalSpeed > 10.0f)
		{
			BobCycle += HorizontalSpeed * Sv_BobSpeed * FrameTime * 0.01f;
			float BobValue = FMath::Sin(BobCycle) * Sv_BobAmount * HorizontalSpeed;
			
			// Apply vertical bobbing
			ViewOffset.Z += BobValue * Sv_BobUp;
		}
		else
		{
			// Fade out bobbing when slow
			BobCycle = 0.0f;
		}
	}

	// Final step: update old buttons for next frame
	OldButtons = Cmd.Buttons;
}

// ============================================================================
// CalcRoll — calculate camera lean based on lateral velocity (strafing)
// ============================================================================
void QMovement::CalcRoll(const FVector& Angles, const FVector& Vel, float RollAngle, float RollSpeed)
{
	if (!Sv_EnableCameraRoll)
	{
		ViewRoll = 0.0f;
		return;
	}
	
	FVector Fwd = Forward;
	FVector Rht = Right;
	FVector U = Up;

	float Side = FVector::DotProduct(Vel, Rht);
	float Sign = Side < 0 ? -1.0f : 1.0f;
	Side = FMath::Abs(Side);
	float Value = RollAngle;

	if (Side < RollSpeed)
	{
		Side = Side * Value / RollSpeed;
	}
	else
	{
		Side = Value;
	}

	// We apply a multiplier of 4.0 like pm_shared.c
	ViewRoll = Side * Sign * 4.0f;
}

// ============================================================================
// SplineFraction — ease-in, ease-out style interpolation for ducking
// ============================================================================
float QMovement::SplineFraction(float Value, float Scale)
{
	Value = Scale * Value;
	float ValueSquared = Value * Value;
	return 3.0f * ValueSquared - 2.0f * ValueSquared * Value;
}

// ============================================================================
// CheckFalling — apply landing punch based on fall velocity
// ============================================================================
void QMovement::CheckFalling()
{
	if (!Sv_EnableCameraPunch)
		return;

	if (bDead || FallVelocity < 350.0f) // PLAYER_FALL_PUNCH_THRESHHOLD
	{
		FallVelocity = 0.0f;
		return;
	}

	if (WaterLevel > 0)
	{
		// Landed in water, no punch
	}
	else
	{
		float fVol = 0.5f;
		if (FallVelocity > 580.0f) // PLAYER_MAX_SAFE_FALL_SPEED
		{
			fVol = 1.0f;
		}
		else if (FallVelocity > 290.0f) // PLAYER_MAX_SAFE_FALL_SPEED / 2
		{
			fVol = 0.85f;
		}
		
		if (fVol > 0.0f)
		{
			// Knock the screen around a little bit, temporary effect
			PunchAngle.X = FallVelocity * 0.013f; // Pitch down axis
			if (PunchAngle.X > 8.0f)
			{
				PunchAngle.X = 8.0f;
			}
		}
	}

	FallVelocity = 0.0f;
}

// ============================================================================
// DropPunchAngle — decay the procedural punch over time
// ============================================================================
void QMovement::DropPunchAngle()
{
	float Len = PunchAngle.Size();
	if (Len <= SMALL_NUMBER)
	{
		PunchAngle = FVector::ZeroVector;
		return;
	}
	
	Len -= (10.0f + Len * 0.5f) * FrameTime;
	Len = FMath::Max(Len, 0.0f);
	
	PunchAngle.Normalize();
	PunchAngle *= Len;
}
