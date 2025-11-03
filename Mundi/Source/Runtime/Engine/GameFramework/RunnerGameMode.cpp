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

		// 플레이어 위치
		FVector PlayerLocation = PlayerPawn->GetActorLocation();

		// 카메라 위치: 플레이어 뒤쪽(-600) 위쪽(+300)
		FVector CameraOffset(-3.0f, 0.0f, 3.0f);
		FVector CameraLocation = PlayerLocation + CameraOffset;

		// 카메라 위치 설정
		Camera->SetActorLocation(CameraLocation);

		//// 카메라가 플레이어를 향하도록 회전
		//FVector LookAtTarget = PlayerLocation + FVector(0.0f, 0.0f, 50.0f); // 플레이어 중심보다 약간 위
		//FVector Direction = (LookAtTarget - CameraLocation).GetNormalized();

		//// Direction으로부터 Rotation 계산
		//float Pitch = asinf(-Direction.Z) * (180.0f / 3.14159265f);
		//float Yaw = atan2f(Direction.Y, Direction.X) * (180.0f / 3.14159265f);

		//Camera->SetActorRotation(FQuat::MakeFromEulerZYX(FVector(Pitch, Yaw, 0.0f)));
	}

	// 게임 진행 중이면 난이도 업데이트 (나중에 구현)
	// if (GetGameState() && GetGameState()->IsMatchInProgress())
	// {
	//     UpdateDifficulty(DeltaSeconds);
	// }
}

void ARunnerGameMode::RestartGame()
{
	UE_LOG("[RunnerGameMode] RestartGame called!");
	// 플레이어 리스폰
	if (PlayerController)
	{
		UE_LOG("[RunnerGameMode] Restarting player...");
		RestartPlayer(PlayerController);

		// RunnerCharacter::BeginPlay()에서 자동으로 스크립트 연결됨
		APawn* SpawnedPawn = PlayerController->GetPawn();
		if (SpawnedPawn)
		{
			UE_LOG("[RunnerGameMode] New Pawn location: (%.1f, %.1f, %.1f)",
				SpawnedPawn->GetActorLocation().X,
				SpawnedPawn->GetActorLocation().Y,
				SpawnedPawn->GetActorLocation().Z);
			UE_LOG("[RunnerGameMode] Player restarted successfully!");
		}
		else
		{
			UE_LOG("[RunnerGameMode] ERROR: Failed to get spawned pawn after restart!");
		}
	}
	// 부모 클래스의 RestartGame 호출 (GameState 초기화 + StartGame)
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
	GameState->SetScore(CoinScore);
	UE_LOG("[RunnerGameMode] Coin Collected! Value: %d, CoinScore: %d", CoinValue, CoinScore);
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
