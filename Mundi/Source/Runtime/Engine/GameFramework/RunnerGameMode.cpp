// ────────────────────────────────────────────────────────────────────────────
// RunnerGameMode.cpp
// 런2 게임 모드 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "RunnerGameMode.h"
#include "Character.h"
#include "PlayerController.h"
#include "Source/Runtime/LuaScripting/UScriptManager.h"

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
	DefaultPawnClass = ACharacter::StaticClass();

	// 플레이어 스폰 위치 (런2 게임 시작 위치)
	PlayerSpawnLocation = FVector(0.0f, 0.0f, 30.0f);

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

	// Player가 스폰된 직후, Character::BeginPlay 이전에 스크립트 부착
	if (PlayerController)
	{
		APawn* SpawnedPawn = PlayerController->GetPawn();
		if (SpawnedPawn)
		{
			UE_LOG("[RunnerGameMode] Attaching PlayerAutoMove.lua to spawned player");

			FLuaLocalValue LuaLocalValue;
			LuaLocalValue.MyActor = SpawnedPawn;
			UScriptManager::GetInstance().AttachScriptTo(LuaLocalValue, "PlayerAutoMove.lua");

			UE_LOG("[RunnerGameMode] PlayerAutoMove.lua attached successfully!");
		}
		else
		{
			UE_LOG("[RunnerGameMode] ERROR: SpawnedPawn is null!");
		}
	}
	else
	{
		UE_LOG("[RunnerGameMode] ERROR: PlayerController is null!");
	}
}

void ARunnerGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 게임 진행 중이면 난이도 업데이트 (나중에 구현)
	// if (GetGameState() && GetGameState()->IsMatchInProgress())
	// {
	//     UpdateDifficulty(DeltaSeconds);
	// }
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 이벤트 (나중에 구현)
// ────────────────────────────────────────────────────────────────────────────

void ARunnerGameMode::OnPlayerDeath(ACharacter* Player)
{
	UE_LOG("[RunnerGameMode] Player Died!");
	// TODO: GameState 업데이트
}

void ARunnerGameMode::OnCoinCollected(int32 CoinValue)
{
	UE_LOG("[RunnerGameMode] Coin Collected! Value: %d", CoinValue);
	// TODO: GameState 업데이트
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
