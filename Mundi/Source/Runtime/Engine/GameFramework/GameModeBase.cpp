#include "pch.h"
#include "GameModeBase.h"
#include "GameStateBase.h"
#include "World.h"

// AGameModeBase를 ObjectFactory에 등록
IMPLEMENT_CLASS(AGameModeBase)

// 프로퍼티 등록
BEGIN_PROPERTIES(AGameModeBase)
	// PlayerSpawnLocation 프로퍼티 등록
	ADD_PROPERTY(FVector, PlayerSpawnLocation, "GameMode", true, "플레이어 스폰 위치입니다.")
END_PROPERTIES()

AGameModeBase::AGameModeBase()
	: GameState(nullptr)
	, PlayerSpawnLocation(FVector(0.0f, 0.0f, 0.0f))
	, bGameStarted(false)
{
	// GameMode는 물리/렌더링 대상이 아님 (Info 상속)
}

void AGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// GameState 자동 생성 (없는 경우)
	if (!GameState.IsValid())
	{
		if (World)
		{
			AGameStateBase* NewGameState = World->SpawnActor<AGameStateBase>();
			if (NewGameState)
			{
				GameState = TWeakPtr<AGameStateBase>(NewGameState);
				UE_LOG("GameMode: GameState 자동 생성됨");
			}
		}
	}
}

void AGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 추가 게임 로직 필요 시 여기에 구현
}

void AGameModeBase::StartGame()
{
	if (bGameStarted)
	{
		UE_LOG("GameMode: 게임이 이미 시작되었습니다.");
		return;
	}

	bGameStarted = true;

	// GameState 상태 변경
	if (GameState.IsValid())
	{
		GameState.Get()->SetGameState(EGameState::Playing);
	}

	// 델리게이트 브로드캐스트
	OnGameStarted.Broadcast();

	UE_LOG("GameMode: 게임 시작");
}

void AGameModeBase::EndGame(bool bVictory)
{
	if (!bGameStarted)
	{
		return;
	}

	// GameState 상태 변경
	if (GameState.IsValid())
	{
		if (bVictory)
		{
			GameState.Get()->SetGameState(EGameState::Victory);
		}
		else
		{
			GameState.Get()->SetGameState(EGameState::GameOver);
		}
	}

	// 델리게이트 브로드캐스트
	OnGameEnded.Broadcast(bVictory);
}

void AGameModeBase::RestartGame()
{
	// GameState 초기화
	if (GameState.IsValid())
	{
		GameState.Get()->SetGameState(EGameState::NotStarted);
		GameState.Get()->SetScore(0);
		GameState.Get()->ResetTimer();
	}

	bGameStarted = false;

	// 델리게이트 브로드캐스트
	OnGameRestarted.Broadcast();

	UE_LOG("GameMode: 게임 재시작");

	// 자동으로 게임 시작
	// StartGame();
}

void AGameModeBase::PauseGame()
{
	if (!bGameStarted)
	{
		UE_LOG("GameMode: 게임이 시작되지 않았습니다.");
		return;
	}

	// GameState 상태 변경
	if (GameState.IsValid())
	{
		GameState.Get()->SetGameState(EGameState::Paused);
	}

	UE_LOG("GameMode: 게임 일시정지!");
}

void AGameModeBase::ResumeGame()
{
	if (!bGameStarted)
	{
		UE_LOG("GameMode: 게임이 시작되지 않았습니다.");
		return;
	}

	// GameState 상태 변경
	if (GameState.IsValid())
	{
		GameState.Get()->SetGameState(EGameState::Playing);
	}

	UE_LOG("GameMode: 게임 재개!");
}

FVector AGameModeBase::GetPlayerSpawnLocation() const
{
	return PlayerSpawnLocation;
}

void AGameModeBase::SetPlayerSpawnLocation(const FVector& Location)
{
	PlayerSpawnLocation = Location;
}

void AGameModeBase::SetGameState(AGameStateBase* NewGameState)
{
	if (NewGameState)
	{
		GameState = TWeakPtr<AGameStateBase>(NewGameState);
		UE_LOG("GameMode: GameState 설정됨");
	}
}

void AGameModeBase::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	// GameMode 복제 시 게임 상태 초기화
	bGameStarted = false;
}
