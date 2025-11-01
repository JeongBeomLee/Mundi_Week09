// ────────────────────────────────────────────────────────────────────────────
// CharacterMovementComponent.cpp
// Character 이동 컴포넌트 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "CharacterMovementComponent.h"
#include "Character.h"

IMPLEMENT_CLASS(UCharacterMovementComponent)

BEGIN_PROPERTIES(UCharacterMovementComponent)
	ADD_PROPERTY(float, MaxWalkSpeed, "Movement", true, "최대 걷기 속도 (cm/s)")
	ADD_PROPERTY(float, JumpZVelocity, "Movement", true, "점프 초기 속도 (cm/s)")
	ADD_PROPERTY(float, GravityScale, "Movement", true, "중력 스케일 (1.0 = 기본 중력)")
END_PROPERTIES()

// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

UCharacterMovementComponent::UCharacterMovementComponent()
	: CharacterOwner(nullptr)
	, Velocity(FVector())
	, PendingInputVector(FVector())
	, MovementMode(EMovementMode::Falling)
	, TimeInAir(0.0f)
	, bIsJumping(false)
	// 이동 설정
	, MaxWalkSpeed(600.0f)           // 6 m/s
	, MaxAcceleration(2048.0f)       // 20.48 m/s²
	, GroundFriction(8.0f)
	, AirControl(0.05f)
	, BrakingDeceleration(2048.0f)
	// 중력 설정
	, GravityScale(1.0f)
	// 점프 설정
	, JumpZVelocity(420.0f)          // 4.2 m/s
	, MaxAirTime(2.0f)
	, bCanJump(true)
{
	bCanEverTick = true;
}

UCharacterMovementComponent::~UCharacterMovementComponent()
{
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void UCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// Owner를 Character로 캐스팅
	CharacterOwner = Cast<ACharacter>(GetOwner());
}

void UCharacterMovementComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	if (!CharacterOwner)
	{
		return;
	}

	// 1. 속도 업데이트 (입력, 마찰, 가속)
	UpdateVelocity(DeltaTime);

	// 2. 중력 적용
	ApplyGravity(DeltaTime);

	// 3. 위치 업데이트
	MoveUpdatedComponent(DeltaTime);

	// 4. 지면 체크
	bool bWasGrounded = IsGrounded();
	bool bIsNowGrounded = CheckGround();

	// 5. 이동 모드 업데이트
	if (bIsNowGrounded && !bWasGrounded)
	{
		// 착지
		SetMovementMode(EMovementMode::Walking);
		Velocity.Z = 0.0f;
		TimeInAir = 0.0f;
		bIsJumping = false;
	}
	else if (!bIsNowGrounded && bWasGrounded)
	{
		// 낙하 시작
		SetMovementMode(EMovementMode::Falling);
	}

	// 6. 공중 시간 체크
	if (IsFalling())
	{
		TimeInAir += DeltaTime;

		// 너무 오래 공중에 있으면 강제로 착지 (안전장치)
		if (TimeInAir > MaxAirTime)
		{
			FVector Location = CharacterOwner->GetActorLocation();
			Location.Z = 0.0f; // 지면으로 이동
			CharacterOwner->SetActorLocation(Location);
			SetMovementMode(EMovementMode::Walking);
			TimeInAir = 0.0f;
		}
	}

	// 입력 초기화
	PendingInputVector = FVector();
}

// ────────────────────────────────────────────────────────────────────────────
// 이동 함수
// ────────────────────────────────────────────────────────────────────────────

void UCharacterMovementComponent::AddInputVector(FVector WorldDirection, float ScaleValue)
{
	if (ScaleValue == 0.0f || WorldDirection.SizeSquared() == 0.0f)
	{
		return;
	}

	// XY 평면으로 제한 (수평 이동만)
	WorldDirection.Z = 0.0f;
	FVector NormalizedDirection = WorldDirection.GetNormalized();

	PendingInputVector += NormalizedDirection * ScaleValue;
}

bool UCharacterMovementComponent::Jump()
{
	// 점프 가능 조건 체크
	if (!bCanJump || !IsGrounded())
	{
		return false;
	}

	// 점프 속도 적용
	Velocity.Z = JumpZVelocity;

	// 이동 모드 변경
	SetMovementMode(EMovementMode::Falling);
	bIsJumping = true;

	return true;
}

void UCharacterMovementComponent::StopJumping()
{
	// 점프 키를 뗐을 때 상승 속도를 줄임
	if (bIsJumping && Velocity.Z > 0.0f)
	{
		Velocity.Z *= 0.5f;
	}
}

void UCharacterMovementComponent::SetMovementMode(EMovementMode NewMode)
{
	if (MovementMode == NewMode)
	{
		return;
	}

	EMovementMode PrevMode = MovementMode;
	MovementMode = NewMode;

	// 모드 전환 시 처리
	if (MovementMode == EMovementMode::Walking)
	{
		// 착지 시 수직 속도 제거
		Velocity.Z = 0.0f;
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 내부 이동 로직
// ────────────────────────────────────────────────────────────────────────────

void UCharacterMovementComponent::UpdateVelocity(float DeltaTime)
{
	if (PendingInputVector.SizeSquared() > 0.0f)
	{
		// 입력이 있으면 가속
		FVector InputDirection = PendingInputVector.GetNormalized();
		float CurrentControl = IsGrounded() ? 1.0f : AirControl;
		float AccelRate = MaxAcceleration * CurrentControl;

		// 목표 속도
		FVector TargetVelocity = InputDirection * MaxWalkSpeed;

		// 수평 속도만 보간 (Z는 중력이 처리)
		FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
		FVector HorizontalTarget(TargetVelocity.X, TargetVelocity.Y, 0.0f);

		// 가속도 적용
		FVector Delta = HorizontalTarget - HorizontalVelocity;
		float DeltaSize = Delta.Size();

		if (DeltaSize > 0.0f)
		{
			FVector AccelDir = Delta / DeltaSize;
			float AccelAmount = FMath::Min(DeltaSize, AccelRate * DeltaTime);

			HorizontalVelocity += AccelDir * AccelAmount;
		}

		// 최대 속도 제한
		if (HorizontalVelocity.Size() > MaxWalkSpeed)
		{
			HorizontalVelocity = HorizontalVelocity.GetNormalized() * MaxWalkSpeed;
		}

		Velocity.X = HorizontalVelocity.X;
		Velocity.Y = HorizontalVelocity.Y;
	}
	else if (IsGrounded())
	{
		// 입력이 없으면 마찰 적용 (지면에서만)
		FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
		float CurrentSpeed = HorizontalVelocity.Size();

		if (CurrentSpeed > 0.0f)
		{
			float FrictionAmount = GroundFriction * BrakingDeceleration * DeltaTime;
			float NewSpeed = FMath::Max(0.0f, CurrentSpeed - FrictionAmount);
			float SpeedRatio = NewSpeed / CurrentSpeed;

			Velocity.X *= SpeedRatio;
			Velocity.Y *= SpeedRatio;
		}
	}
}

void UCharacterMovementComponent::ApplyGravity(float DeltaTime)
{
	// 지면에 있으면 중력 적용 안 함
	if (IsGrounded())
	{
		return;
	}

	// 중력 가속도 적용
	float Gravity = DefaultGravity * GravityScale;
	Velocity.Z += Gravity * DeltaTime;

	// 최대 낙하 속도 제한 (터미널 속도)
	constexpr float MaxFallSpeed = -4000.0f; // -40 m/s
	if (Velocity.Z < MaxFallSpeed)
	{
		Velocity.Z = MaxFallSpeed;
	}
}

void UCharacterMovementComponent::MoveUpdatedComponent(float DeltaTime)
{
	if (Velocity.SizeSquared() == 0.0f)
	{
		return;
	}

	// 새 위치 계산
	FVector CurrentLocation = CharacterOwner->GetActorLocation();
	FVector Delta = Velocity * DeltaTime;
	FVector NewLocation = CurrentLocation + Delta;

	// 위치 업데이트
	CharacterOwner->SetActorLocation(NewLocation);
}

bool UCharacterMovementComponent::CheckGround()
{
	if (!CharacterOwner)
	{
		return false;
	}

	FVector Location = CharacterOwner->GetActorLocation();

	// 간단한 지면 체크: Y = 0을 지면으로 가정
	// 나중에 충돌 시스템과 연결하여 제대로 구현
	constexpr float GroundLevel = 0.0f;
	constexpr float GroundTolerance = 1.0f; // 1cm 오차 허용

	// 지면 근처에 있고, 아래로 떨어지지 않고 있으면 지면에 있음
	bool bNearGround = Location.Z <= (GroundLevel + GroundTolerance);
	bool bNotRising = Velocity.Z <= 0.0f;

	if (bNearGround && bNotRising)
	{
		// 지면에 정확히 맞춤
		Location.Z = GroundLevel;
		CharacterOwner->SetActorLocation(Location);
		return true;
	}

	return false;
}
