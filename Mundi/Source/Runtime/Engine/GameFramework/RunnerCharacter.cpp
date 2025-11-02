// ────────────────────────────────────────────────────────────────────────────
// RunnerCharacter.cpp
// Runner 게임용 캐릭터 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "RunnerCharacter.h"
#include "CharacterMovementComponent.h"
#include "InputComponent.h"
#include "World.h"
#include "GameModeBase.h"

IMPLEMENT_CLASS(ARunnerCharacter)

BEGIN_PROPERTIES(ARunnerCharacter)
	MARK_AS_SPAWNABLE("RunnerCharacter", "Runner 게임 전용 캐릭터 클래스입니다. Lua 스크립트로 제어됩니다.")
END_PROPERTIES()

// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

ARunnerCharacter::ARunnerCharacter()
{
	// 기본 설정 (Lua에서 오버라이드 가능)
	if (CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed = 500.0f;
		CharacterMovement->JumpZVelocity = 600.0f;
		CharacterMovement->GravityScale = 1.5f;
		CharacterMovement->AirControl = 0.3f;
	}
}

ARunnerCharacter::~ARunnerCharacter()
{
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void ARunnerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG("[RunnerCharacter] BeginPlay");

	// Lua 스크립트 자동 연결
	FLuaLocalValue LuaLocalValue;
	LuaLocalValue.MyActor = this;
	LuaLocalValue.GameMode = World ? World->GetGameMode() : nullptr;

	UScriptManager::GetInstance().AttachScriptTo(LuaLocalValue, "RunnerCharacter.lua");
	UE_LOG("[RunnerCharacter] Auto-attached Lua script: RunnerCharacter.lua");
}

// ────────────────────────────────────────────────────────────────────────────
// 유틸리티 함수
// ────────────────────────────────────────────────────────────────────────────

FVector ARunnerCharacter::GetUpDirection() const
{
	if (CharacterMovement)
	{
		// 중력 반대 방향 = 위쪽
		return CharacterMovement->GetGravityDirection() * -1.0f;
	}
	return FVector(0.0f, 0.0f, 1.0f); // 기본값: Z 위쪽
}

FVector ARunnerCharacter::GetRightDirection() const
{
	FVector ForwardDir = GetForwardDirection();
	FVector UpDir = GetUpDirection();

	// 전진 방향과 위쪽에 수직인 벡터 = 우측 방향
	FVector RightDir = FVector::Cross(UpDir, ForwardDir);

	// 만약 평행하면 다른 축 사용
	if (RightDir.SizeSquared() < 0.01f)
	{
		RightDir = FVector::Cross(UpDir, FVector(0.0f, 1.0f, 0.0f));
	}

	return RightDir.GetNormalized();
}

FVector ARunnerCharacter::GetForwardDirection() const
{
	// Runner 게임에서는 항상 X축 양의 방향으로 전진
	return FVector(1.0f, 0.0f, 0.0f);
}

// ────────────────────────────────────────────────────────────────────────────
// 중력 방향 제어
// ────────────────────────────────────────────────────────────────────────────

void ARunnerCharacter::SetGravityDirection(const FVector& NewGravityDir)
{
	if (CharacterMovement)
	{
		CharacterMovement->SetGravityDirection(NewGravityDir);
		UE_LOG("[RunnerCharacter] Gravity direction changed to: (%.2f, %.2f, %.2f)",
			NewGravityDir.X, NewGravityDir.Y, NewGravityDir.Z);
	}
}

FVector ARunnerCharacter::GetGravityDirection() const
{
	if (CharacterMovement)
	{
		return CharacterMovement->GetGravityDirection();
	}
	return FVector(0.0f, 0.0f, -1.0f);
}

