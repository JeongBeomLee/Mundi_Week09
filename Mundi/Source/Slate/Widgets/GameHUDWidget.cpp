#include "pch.h"
#include "GameHUDWidget.h"
#include "GameStateBase.h"
#include <sstream>
#include <iomanip>

IMPLEMENT_CLASS(UGameHUDWidget)

UGameHUDWidget::UGameHUDWidget()
	: GameState(nullptr)
	, GameStateChangedHandle(0)
	, ScoreChangedHandle(0)
	, TimerUpdatedHandle(0)
	, CachedScore(0)
	, CachedElapsedTime(0.0f)
	, CachedGameStateText("Not Started")
{
}

void UGameHUDWidget::Initialize()
{
	// 초기화 (GameState는 나중에 SetGameState()로 설정됨)
}

void UGameHUDWidget::Update()
{
	// 델리게이트를 통해 자동으로 업데이트되므로 여기서는 아무것도 하지 않음
	// 필요시 추가 로직 작성 가능
}

void UGameHUDWidget::RenderWidget()
{
	// PIE 모드가 아니면 렌더링하지 않음
	if (!GWorld || !GWorld->bPie)
		return;

	// 화면 상단 중앙에 HUD 배치
	ImGuiIO& io = ImGui::GetIO();
	const float windowWidth = 300.0f;
	const float windowHeight = 100.0f;
	const float padding = 100.0f;

	ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - windowWidth) * 0.01f, padding), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

	// 윈도우 플래그: 타이틀바 없음, 리사이징 불가, 이동 불가, 반투명 배경
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings;

	// 반투명 배경
	ImGui::SetNextWindowBgAlpha(0.7f);

	if (ImGui::Begin("##GameHUD", nullptr, windowFlags))
	{
		// 중앙 정렬 설정
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));

		// 게임 상태 표시 (상단)
		ImVec4 stateColor = GetGameStateColor();
		ImGui::PushStyleColor(ImGuiCol_Text, stateColor);
		float textWidth = ImGui::CalcTextSize(CachedGameStateText.c_str()).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::TextUnformatted(CachedGameStateText.c_str());
		ImGui::PopStyleColor();

		ImGui::Separator();

		// 스코어 표시
		FString scoreText = "Score: " + std::to_string(CachedScore);
		textWidth = ImGui::CalcTextSize(scoreText.c_str()).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::TextUnformatted(scoreText.c_str());

		// 타이머 표시 (MM:SS 형식)
		FString timeText = "Time: " + FormatTime(CachedElapsedTime);
		textWidth = ImGui::CalcTextSize(timeText.c_str()).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::TextUnformatted(timeText.c_str());

		ImGui::PopStyleVar();
	}
	ImGui::End();
}

void UGameHUDWidget::SetGameState(AGameStateBase* InGameState)
{
	// 기존 델리게이트 바인딩 해제
	UnbindDelegates();

	if (InGameState)
	{
		GameState = TWeakPtr<AGameStateBase>(InGameState);

		// 델리게이트 바인딩 (핸들 저장)
		GameStateChangedHandle = InGameState->OnGameStateChanged.AddDynamic(this, &UGameHUDWidget::OnGameStateChanged_Handler);
		ScoreChangedHandle = InGameState->OnScoreChanged.AddDynamic(this, &UGameHUDWidget::OnScoreChanged_Handler);
		TimerUpdatedHandle = InGameState->OnTimerUpdated.AddDynamic(this, &UGameHUDWidget::OnTimerUpdated_Handler);

		// 초기 데이터 캐시
		CachedScore = InGameState->GetScore();
		CachedElapsedTime = InGameState->GetElapsedTime();
		CachedGameStateText = GetGameStateText();
	}
	else
	{
		GameState.Reset();

		// 초기값으로 리셋
		CachedScore = 0;
		CachedElapsedTime = 0.0f;
		CachedGameStateText = "No GameState";
	}
}

void UGameHUDWidget::UnbindDelegates()
{
	if (GameState.IsValid())
	{
		AGameStateBase* State = GameState.Get();
		// 이 위젯의 바인딩만 제거 (다른 위젯의 바인딩은 유지)
		State->OnGameStateChanged.RemoveDynamic(GameStateChangedHandle);
		State->OnScoreChanged.RemoveDynamic(ScoreChangedHandle);
		State->OnTimerUpdated.RemoveDynamic(TimerUpdatedHandle);
	}
}

void UGameHUDWidget::OnGameStateChanged_Handler(EGameState OldState, EGameState NewState)
{
	// 게임 상태 텍스트 업데이트
	CachedGameStateText = GetGameStateText();
	UE_LOG("GameHUDWidget: GameState changed from %d to %d", static_cast<int>(OldState), static_cast<int>(NewState));
}

void UGameHUDWidget::OnScoreChanged_Handler(int32 OldScore, int32 NewScore)
{
	// 스코어 캐시 업데이트
	CachedScore = NewScore;
	UE_LOG("GameHUDWidget: Score changed from %d to %d", OldScore, NewScore);
}

void UGameHUDWidget::OnTimerUpdated_Handler(float ElapsedTime)
{
	// 타이머 캐시 업데이트
	CachedElapsedTime = ElapsedTime;
}

FString UGameHUDWidget::FormatTime(float Seconds) const
{
	int32 Minutes = static_cast<int32>(Seconds) / 60;
	int32 Secs = static_cast<int32>(Seconds) % 60;

	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << Minutes << ":" << std::setw(2) << Secs;
	return oss.str();
}

FString UGameHUDWidget::GetGameStateText() const
{
	if (!GameState.IsValid())
		return "No GameState";

	EGameState State = GameState.Get()->GetGameState();
	switch (State)
	{
	case EGameState::NotStarted:
		return "Not Started";
	case EGameState::Playing:
		return "Playing";
	case EGameState::Paused:
		return "Paused";
	case EGameState::GameOver:
		return "Game Over";
	case EGameState::Victory:
		return "Victory!";
	default:
		return "Unknown";
	}
}

ImVec4 UGameHUDWidget::GetGameStateColor() const
{
	if (!GameState.IsValid())
		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 흰색

	EGameState State = GameState.Get()->GetGameState();
	switch (State)
	{
	case EGameState::NotStarted:
		return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // 회색
	case EGameState::Playing:
		return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 녹색
	case EGameState::Paused:
		return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
	case EGameState::GameOver:
		return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색
	case EGameState::Victory:
		return ImVec4(0.0f, 0.8f, 1.0f, 1.0f); // 하늘색
	default:
		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 흰색
	}
}
