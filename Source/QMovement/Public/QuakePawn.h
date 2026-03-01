// QuakePawn.h — APawn using QMovement (GoldSrc pmove.c port)
//
// Enhanced Input (WASD + Mouse + Jump) → QMovement.ProcessMovement() → SetActorLocation
// Movement settings editable via UMovementSettings Data Asset or console commands (sv_*).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "QMovement.h"
#include "QuakePawn.generated.h"

class UCapsuleComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UMovementSettings;

UCLASS()
class QMOVEMENT_API AQuakePawn : public APawn
{
	GENERATED_BODY()

public:
	AQuakePawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// =========================================================================
	// Components
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> CameraComp;

	// =========================================================================
	// Movement engine (GoldSrc pmove.c port)
	// =========================================================================

	/** The QMovement instance — runtime movement engine */
	QMovement MoveEngine;

	// =========================================================================
	// Movement Settings Data Asset
	// =========================================================================

	/**
	 * Assign a UMovementSettings Data Asset here to configure all movement values.
	 * If left empty (nullptr), CS 1.6 defaults are used automatically.
	 *
	 * To create: Content Browser → Right-click → Miscellaneous → Data Asset → "MovementSettings"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quake Movement")
	TObjectPtr<UMovementSettings> MovementSettings;

	// =========================================================================
	// Mouse sensitivity
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quake Movement|Input")
	float MouseSensitivity = 1.0f;

	// =========================================================================
	// Enhanced Input assets (assign in Blueprint or Details panel)
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DuckAction;

	// =========================================================================
	// Debug
	// =========================================================================

	/** Show velocity on screen (useful for testing bhop speed gains) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quake Movement|Debug")
	bool bShowDebugSpeed = true;

	// =========================================================================
	// Console commands — type in UE console (~) like in CS: sv_gravity 800
	// =========================================================================

	UFUNCTION(Exec) void sv_gravity(const FString& Args);
	UFUNCTION(Exec) void sv_maxspeed(const FString& Args);
	UFUNCTION(Exec) void sv_airaccelerate(const FString& Args);
	UFUNCTION(Exec) void sv_accelerate(const FString& Args);
	UFUNCTION(Exec) void sv_friction(const FString& Args);
	UFUNCTION(Exec) void sv_jumpspeed(const FString& Args);
	UFUNCTION(Exec) void sv_stopspeed(const FString& Args);
	UFUNCTION(Exec) void sv_stepsize(const FString& Args);
	UFUNCTION(Exec) void sv_airspeedcap(const FString& Args);
	UFUNCTION(Exec) void sv_maxvelocity(const FString& Args);
	UFUNCTION(Exec) void sv_autobhop(const FString& Args);
	UFUNCTION(Exec) void sv_disablebhopcap(const FString& Args);
	UFUNCTION(Exec) void sv_camerapunch(const FString& Args);
	UFUNCTION(Exec) void sv_cameraroll(const FString& Args);
	UFUNCTION(Exec) void cl_bob(const FString& Args);
	UFUNCTION(Exec) void noclip();

private:
	// Input state
	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bWantsJump = false;
	bool bWantsCrouch = false;

	// Input callbacks
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpPressed(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnDuckPressed(const FInputActionValue& Value);
	void OnDuckReleased(const FInputActionValue& Value);
};
