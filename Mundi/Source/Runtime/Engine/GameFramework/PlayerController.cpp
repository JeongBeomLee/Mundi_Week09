// ────────────────────────────────────────────────────────────────────────────
// PlayerController.cpp
// 플레이어 입력 처리 Controller 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "PlayerController.h"
#include "Pawn.h"
#include "InputComponent.h"
#include "InputManager.h"

IMPLEMENT_CLASS(APlayerController)

BEGIN_PROPERTIES(APlayerController)
	MARK_AS_SPAWNABLE("PlayerController", "플레이어 입력을 처리하는 Controller입니다.")
END_PROPERTIES()

// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

APlayerController::APlayerController()
	: InputManager(nullptr)
	, bInputEnabled(true)
	, bShowMouseCursor(true)
	, bIsMouseLocked(false)
	, MouseSensitivity(1.0f)
{
}

APlayerController::~APlayerController()
{
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void APlayerController::BeginPlay()
{
	Super::BeginPlay();

	// InputManager 참조 획득
	InputManager = &UInputManager::GetInstance();
}

void APlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 입력 처리
	if (bInputEnabled)
	{
		ProcessPlayerInput();
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Pawn 빙의 관련
// ────────────────────────────────────────────────────────────────────────────

void APlayerController::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);
}

void APlayerController::UnPossess()
{
	Super::UnPossess();
}

void APlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		// Pawn의 InputComponent에 입력 바인딩 설정
		UInputComponent* InputComp = InPawn->GetInputComponent();
		if (InputComp)
		{
			InPawn->SetupPlayerInputComponent(InputComp);
		}
	}
}

void APlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

// ────────────────────────────────────────────────────────────────────────────
// 입력 처리
// ────────────────────────────────────────────────────────────────────────────

void APlayerController::ProcessPlayerInput()
{
	if (!PossessedPawn)
	{
		return;
	}

	// Pawn의 InputComponent에 입력 전달
	UInputComponent* InputComp = PossessedPawn->GetInputComponent();
	if (InputComp)
	{
		InputComp->ProcessInput();
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 마우스 커서 제어
// ────────────────────────────────────────────────────────────────────────────

void APlayerController::ShowMouseCursor()
{
	bShowMouseCursor = true;

	if (InputManager)
	{
		InputManager->SetCursorVisible(true);
	}
}

void APlayerController::HideMouseCursor()
{
	bShowMouseCursor = false;

	if (InputManager)
	{
		InputManager->SetCursorVisible(false);
	}
}

void APlayerController::LockMouseCursor()
{
	bIsMouseLocked = true;

	if (InputManager)
	{
		InputManager->LockCursor();
	}
}

void APlayerController::UnlockMouseCursor()
{
	bIsMouseLocked = false;

	if (InputManager)
	{
		InputManager->ReleaseCursor();
	}
}
