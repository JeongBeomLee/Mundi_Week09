// ────────────────────────────────────────────────────────────────────────────
// Character.cpp
// Character 클래스 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "Character.h"
#include "CharacterMovementComponent.h"
#include "SceneComponent.h"
#include "InputComponent.h"
#include "ObjectFactory.h"

IMPLEMENT_CLASS(ACharacter)

BEGIN_PROPERTIES(ACharacter)
	MARK_AS_SPAWNABLE("Character", "이동, 점프 등의 기능을 가진 Character 클래스입니다.")
END_PROPERTIES()

// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

ACharacter::ACharacter()
	: CharacterMovement(nullptr)
	, MeshComponent(nullptr)
	, bIsCrouched(false)
	, CrouchedHeightRatio(0.5f)
{
	// CharacterMovementComponent 생성
	CharacterMovement = CreateDefaultSubobject<UCharacterMovementComponent>("CharacterMovement");
	if (CharacterMovement)
	{
		CharacterMovement->SetOwner(this);
	}

	// Mesh 컴포넌트 생성 (일단 빈 SceneComponent)
	MeshComponent = CreateDefaultSubobject<USceneComponent>("MeshComponent");
	if (MeshComponent)
	{
		MeshComponent->SetOwner(this);
		SetRootComponent(MeshComponent);
	}
}

ACharacter::~ACharacter()
{
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void ACharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ACharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

// ────────────────────────────────────────────────────────────────────────────
// 입력 바인딩
// ────────────────────────────────────────────────────────────────────────────

void ACharacter::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	// 기본 입력 바인딩 예시 (파생 클래스에서 오버라이드 가능)
	//
	// 이동
	// InInputComponent->BindAxis("MoveForward", 'W', 1.0f, this, &ACharacter::MoveForward);
	// InInputComponent->BindAxis("MoveForward", 'S', -1.0f, this, &ACharacter::MoveForward);
	// InInputComponent->BindAxis("MoveRight", 'D', 1.0f, this, &ACharacter::MoveRight);
	// InInputComponent->BindAxis("MoveRight", 'A', -1.0f, this, &ACharacter::MoveRight);
	//
	// 점프
	// InInputComponent->BindAction("Jump", VK_SPACE, this, &ACharacter::Jump, &ACharacter::StopJumping);
	//
	// 회전
	// InInputComponent->BindAxis("Turn", VK_RIGHT, 1.0f, this, &ACharacter::Turn);
	// InInputComponent->BindAxis("Turn", VK_LEFT, -1.0f, this, &ACharacter::Turn);
}

// ────────────────────────────────────────────────────────────────────────────
// 이동 입력 처리
// ────────────────────────────────────────────────────────────────────────────

void ACharacter::AddMovementInput(FVector WorldDirection, float ScaleValue)
{
	if (CharacterMovement)
	{
		CharacterMovement->AddInputVector(WorldDirection, ScaleValue);
	}
}

void ACharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// Forward 방향으로 이동
		FVector Forward = GetActorForward();
		AddMovementInput(Forward, Value);
	}
}

void ACharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// Right 방향으로 이동
		FVector Right = GetActorRight();
		AddMovementInput(Right, Value);
	}
}

void ACharacter::Turn(float Value)
{
	if (Value != 0.0f)
	{
		AddControllerYawInput(Value);
	}
}

void ACharacter::LookUp(float Value)
{
	if (Value != 0.0f)
	{
		AddControllerPitchInput(Value);
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Character 동작
// ────────────────────────────────────────────────────────────────────────────

void ACharacter::Jump()
{
	if (CharacterMovement)
	{
		CharacterMovement->Jump();
	}
}

void ACharacter::StopJumping()
{
	if (CharacterMovement)
	{
		CharacterMovement->StopJumping();
	}
}

bool ACharacter::CanJump() const
{
	return CharacterMovement && CharacterMovement->bCanJump && IsGrounded();
}

void ACharacter::Crouch()
{
	if (bIsCrouched)
	{
		return;
	}

	bIsCrouched = true;

	// 웅크릴 때 이동 속도 감소 (옵션)
	if (CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed *= CrouchedHeightRatio;
	}
}

void ACharacter::UnCrouch()
{
	if (!bIsCrouched)
	{
		return;
	}

	bIsCrouched = false;

	// 이동 속도 복원
	if (CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed /= CrouchedHeightRatio;
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 상태 조회
// ────────────────────────────────────────────────────────────────────────────

FVector ACharacter::GetVelocity() const
{
	if (CharacterMovement)
	{
		return CharacterMovement->GetVelocity();
	}

	return FVector();
}

bool ACharacter::IsGrounded() const
{
	return CharacterMovement && CharacterMovement->IsGrounded();
}

bool ACharacter::IsFalling() const
{
	return CharacterMovement && CharacterMovement->IsFalling();
}
