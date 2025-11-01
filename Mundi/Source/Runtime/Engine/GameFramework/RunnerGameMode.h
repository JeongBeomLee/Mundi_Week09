// ────────────────────────────────────────────────────────────────────────────
// RunnerGameMode.h
// 런2 게임 전용 GameMode
// ────────────────────────────────────────────────────────────────────────────
#pragma once

#include "GameModeBase.h"

// 전방 선언
class ARunnerGameState;
class ACharacter;

/**
 * ARunnerGameMode
 *
 * 런2 게임의 규칙과 로직을 정의합니다.
 * RunnerCharacter를 기본 Pawn으로 사용합니다.
 */
class ARunnerGameMode : public AGameModeBase
{
public:
	DECLARE_CLASS(ARunnerGameMode, AGameModeBase)
	GENERATED_REFLECTION_BODY()
	DECLARE_DUPLICATE(ARunnerGameMode)

	ARunnerGameMode();
	virtual ~ARunnerGameMode() override;

	// ────────────────────────────────────────────────
	// 게임 이벤트
	// ────────────────────────────────────────────────

	/** 플레이어 사망 */
	void OnPlayerDeath(ACharacter* Player);

	/** 코인 수집 */
	void OnCoinCollected(int32 CoinValue);

	/** 장애물 회피 */
	void OnObstacleAvoided();

	/** 점프 */
	void OnPlayerJump();

	// ────────────────────────────────────────────────
	// RunnerGameState 접근 (나중에 추가)
	// ────────────────────────────────────────────────

	// ARunnerGameState* GetRunnerGameState() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 난이도 설정 */
	float BaseDifficulty = 1.0f;
	float DifficultyIncreaseRate = 0.1f;

	/** 점수 설정 */
	int32 JumpScore = 10;
	int32 CoinScore = 50;
	int32 AvoidScore = 20;
};
