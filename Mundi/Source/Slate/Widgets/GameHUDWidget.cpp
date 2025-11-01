#include "pch.h"
#include "GameHUDWidget.h"
#include "GameStateBase.h"
#include <sstream>
#include <iomanip>

IMPLEMENT_CLASS(UGameHUDWidget)

UGameHUDWidget::UGameHUDWidget()
	: GameState(nullptr)
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
	if (!GWorld || !GWorld->bPie)
		return;

	// GameState에서 데이터 읽어오기
	if (GameState.IsValid())
	{
		AGameStateBase* State = GameState.Get();
		CachedScore = State->GetScore();
		CachedElapsedTime = State->GetElapsedTime();
		CachedGameStateText = GetGameStateText();
	}
	else
	{
		// GameState가 없으면 초기값
		CachedScore = 0;
		CachedElapsedTime = 0.0f;
		CachedGameStateText = "No GameState";
	}
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
	const float padding = 20.0f;

	ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - windowWidth) * 0.5f, padding), ImGuiCond_Always);
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
	if (InGameState)
	{
		GameState = TWeakPtr<AGameStateBase>(InGameState);
	}
	else
	{
		GameState.Reset();
	}
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
