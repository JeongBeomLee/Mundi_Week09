// ────────────────────────────────────────────────────────────────────────────
// RunnerGameMode.cpp
// 런2 게임 모드 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "RunnerGameMode.h"
#include "Character.h"
#include "RunnerCharacter.h"
#include "PlayerController.h"
#include "CameraActor.h"
#include "GameStateBase.h"

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

	UE_LOG("[RunnerGameMode] BeginPlay - Runner Game Starting!");

	// RunnerCharacter가 자체적으로 스크립트를 연결하므로 여기서는 불필요
}

void ARunnerGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 카메라를 플레이어 뒤에서 따라가도록 업데이트
	if (PlayerController && PlayerController->GetPawn() && World->GetCameraActor())
	{
		APawn* PlayerPawn = PlayerController->GetPawn();
		ACameraActor* Camera = World->GetCameraActor();

		// RunnerCharacter로 캐스팅 (중력 방향 정보 필요)
		ARunnerCharacter* RunnerChar = dynamic_cast<ARunnerCharacter*>(PlayerPawn);
		if (RunnerChar)
		{
			// 플레이어 위치
			FVector PlayerLocation = PlayerPawn->GetActorLocation();

			// 캐릭터의 로컬 좌표계 방향 가져오기
			FVector UpDirection = RunnerChar->GetUpDirection();        // 중력 반대 = 위
			FVector ForwardDirection = RunnerChar->GetForwardDirection(); // 전진 방향

			// 로컬 좌표계 기준 오프셋 계산
			// 뒤쪽(-Forward) 3.0, 위쪽(+Up) 3.0
			FVector CameraOffset = ForwardDirection * -5.0f + UpDirection * 5.0f;
			FVector CameraLocation = PlayerLocation + CameraOffset;

			// 카메라 위치 설정
			Camera->SetActorLocation(CameraLocation);
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
