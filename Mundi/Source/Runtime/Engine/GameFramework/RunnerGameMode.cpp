// ────────────────────────────────────────────────────────────────────────────
// RunnerGameMode.cpp
// 런2 게임 모드 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "RunnerGameMode.h"
#include "Character.h"
#include "RunnerCharacter.h"
#include "PlayerController.h"
#include "GameStateBase.h"
#include "PlayerCameraManager.h"

IMPLEMENT_CLASS(ARunnerGameMode)

BEGIN_PROPERTIES(ARunnerGameMode)
	MARK_AS_SPAWNABLE("RunnerGameMode", "런2 게임 전용 GameMode입니다.")
	ADD_PROPERTY(float, DifficultyIncreaseRate, "Difficulty", true, "난이도 증가율 (10초마다)")
	ADD_PROPERTY(int32, JumpScore, "Score", true, "점프 시 획득 점수")
	ADD_PROPERTY(int32, CoinScore, "Score", true, "코인 획득 점수")
	ADD_PROPERTY(int32, AvoidScore, "Score", true, "장애물 회피 점수")
END_PROPERTIES()

ARunnerGameMode::ARunnerGameMode()
{
	// RunnerGameState 사용 (나중에 추가)
	// GameStateClass = ARunnerGameState::StaticClass();

	// 기본 PlayerController 사용
	PlayerControllerClass = APlayerController::StaticClass();

	// Character를 기본 Pawn으로 설정
	DefaultPawnClass = ARunnerCharacter::StaticClass();

	// 플레이어 스폰 위치 (런2 게임 시작 위치)
	PlayerSpawnLocation = FVector(0.0f, 0.0f, 3.0f);

	// 자동 스폰 활성화
	bAutoSpawnPlayer = true;

	UE_LOG("[RunnerGameMode] Constructor - DefaultPawnClass set to ACharacter");
}

ARunnerGameMode::~ARunnerGameMode()
{
}

void ARunnerGameMode::BeginPlay()
{
	Super::BeginPlay();  // ← 여기서 InitPlayer()가 호출되어 Character 스폰됨

	// RunnerCharacter가 자체적으로 스크립트를 연결하므로 여기서는 불필요

	// 카메라 트랜지션 효과 적용 테스트 코드
	// PlayerController와 CameraManager 가져오기
	APlayerController* PlayerController = GetPlayerController();
	if (PlayerController)
	{
		APlayerCameraManager* CameraManager = PlayerController->GetPlayerCameraManager();
		if (!CameraManager)
		{
			UE_LOG("[RunnerGameMode] WARNING: CameraManager not found");
			return;
		}

		// 플레이어 캐릭터 위치 가져오기
		APawn* PlayerPawn = PlayerController->GetPawn();
		if (PlayerPawn)
		{
			FVector PlayerLocation = PlayerPawn->GetActorLocation();

			// 시작 카메라 위치: 플레이어 위 + 뒤에서 내려다보는 시점
			FVector StartLocation = PlayerLocation + FVector(-8.0f, 0.0f, 6.0f);
			FQuat StartRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, -45.0f, 0.0f)); // 45도 아래로 (Pitch)
			float StartFOV = 90.0f;

			// 1. ViewTarget을 플레이어로 설정 (Target 설정)
			CameraManager->SetViewTarget(PlayerPawn);

			// 2. 카메라 초기 위치 덮어쓰기 (블렌딩 없이)
			CameraManager->SetCameraTransform(StartLocation, StartRotation, StartFOV);

			// 3. 프리셋을 사용하여 플레이어 카메라로 부드럽게 전환, "Cinematic" 프리셋 사용
			CameraManager->SetViewTargetWithBlendPreset(PlayerPawn, "Cinematic");
		}
	}
}

void ARunnerGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 사운드 지연 재생 처리
	if (bWaitingForSecondSound)
	{
		SoundDelayTimer += DeltaSeconds;
		if (SoundDelayTimer >= 1.5f)
		{
			// 두 번째 사운드 재생
			USoundManager::GetInstance().PlaySound2D("Data/Sounds/InfinityRunner/GameOverSoundFx.mp3", false, ESoundChannelType::SFX);

			bWaitingForSecondSound = false;
			SoundDelayTimer = 0.0f;
		}
	}
}

void ARunnerGameMode::RestartGame()
{
	Super::RestartGame();
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 이벤트 (나중에 구현)
// ────────────────────────────────────────────────────────────────────────────

void ARunnerGameMode::OnPlayerDeath(ACharacter* Player)
{
	UE_LOG("[RunnerGameMode] Player Died!");

	// 첫 번째 사운드 즉시 재생
	USoundManager::GetInstance().PlaySound2D("Data/Sounds/InfinityRunner/FailedSoundFx.mp3", false, ESoundChannelType::SFX);

	// 1.5초 후 두 번째 사운드 재생을 위한 타이머 시작
	bWaitingForSecondSound = true;
	SoundDelayTimer = 0.0f;

	// GameState를 GameOver로 변경
	if (GameState)
	{
		GameState->SetGameState(EGameState::GameOver);
	}
}

void ARunnerGameMode::OnCoinCollected(int32 CoinValue)
{
	
	// TODO: GameState 업데이트
	CoinScore += CoinValue;
	if (GameState)
	{
		GameState->SetScore(CoinScore);
		UE_LOG("[RunnerGameMode] Coin Collected! Value: %d, CoinScore: %d", CoinValue, CoinScore);
	}
	else
	{
		UE_LOG("[RunnerGameMode] ERROR: GameState is null when collecting coin!");
	}
}

void ARunnerGameMode::OnObstacleAvoided()
{
	UE_LOG("[RunnerGameMode] Obstacle Avoided!");
	// TODO: GameState 업데이트
}

void ARunnerGameMode::OnPlayerJump()
{
	UE_LOG("[RunnerGameMode] Player Jumped!");
	// TODO: GameState 업데이트
}
