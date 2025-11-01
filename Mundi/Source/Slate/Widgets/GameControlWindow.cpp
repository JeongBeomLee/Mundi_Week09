#include "pch.h"
#include "GameControlWindow.h"
#include "GameModeBase.h"
#include "GameStateBase.h"

IMPLEMENT_CLASS(UGameControlWindow)

UGameControlWindow::UGameControlWindow()
	: GameMode(nullptr)
{
}

void UGameControlWindow::Initialize()
{
	// 초기화 (GameMode는 나중에 SetGameMode()로 설정됨)
}

void UGameControlWindow::Update()
{
	// 필요시 상태 업데이트
}

void UGameControlWindow::RenderWidget()
{
	// PIE 모드가 아니면 렌더링하지 않음
	if (!GWorld || !GWorld->bPie)
		return;

	// GameMode가 없으면 렌더링하지 않음
	if (!GameMode.IsValid())
		return;

	// 화면 우측 하단에 배치
	ImGuiIO& io = ImGui::GetIO();
	const float windowWidth = 240.0f;
	const float windowHeight = 280.0f;  // 스코어 테스트 버튼 추가로 높이 증가
	const float padding = 20.0f;

	ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - windowWidth) * 0.01f, io.DisplaySize.y - windowHeight - padding), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

	// 윈도우 플래그
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings;

	// 반투명 배경
	ImGui::SetNextWindowBgAlpha(0.85f);

	if (ImGui::Begin("Game Controls", nullptr, windowFlags))
	{
		AGameModeBase* Mode = GameMode.Get();

		bool bStartEnabled = IsStartButtonEnabled();
		bool bPauseEnabled = IsPauseButtonEnabled();
		bool bResumeEnabled = IsResumeButtonEnabled();
		bool bRestartEnabled = IsRestartButtonEnabled();
		bool bEndGameEnabled = IsEndGameButtonEnabled();

		// 버튼 스타일 설정
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));

		// 버튼 크기
		ImVec2 buttonSize(windowWidth - 30.0f, 30.0f);

		// Start Game 버튼
		if (!bStartEnabled)
			ImGui::BeginDisabled();

		if (ImGui::Button("Start Game", buttonSize))
		{
			Mode->StartGame();
		}

		if (!bStartEnabled)
			ImGui::EndDisabled();

		// Pause Game 버튼
		if (!bPauseEnabled)
			ImGui::BeginDisabled();

		if (ImGui::Button("Pause Game", buttonSize))
		{
			Mode->PauseGame();
		}

		if (!bPauseEnabled)
			ImGui::EndDisabled();

		// Resume Game 버튼
		if (!bResumeEnabled)
			ImGui::BeginDisabled();

		if (ImGui::Button("Resume Game", buttonSize))
		{
			Mode->ResumeGame();
		}

		if (!bResumeEnabled)
			ImGui::EndDisabled();

		// Restart Game 버튼
		if (!bRestartEnabled)
			ImGui::BeginDisabled();

		if (ImGui::Button("Restart Game", buttonSize))
		{
			Mode->RestartGame();
		}

		if (!bRestartEnabled)
			ImGui::EndDisabled();

		// End Game (Victory) 버튼
		if (!bEndGameEnabled)
			ImGui::BeginDisabled();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.9f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.5f, 0.7f, 1.0f));

		if (ImGui::Button("End Game (Victory)", buttonSize))
		{
			Mode->EndGame(true);
		}

		ImGui::PopStyleColor(3);

		if (!bEndGameEnabled)
			ImGui::EndDisabled();

		// End Game (Game Over) 버튼
		if (!bEndGameEnabled)
			ImGui::BeginDisabled();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));

		if (ImGui::Button("End Game (Game Over)", buttonSize))
		{
			Mode->EndGame(false);
		}

		ImGui::PopStyleColor(3);

		if (!bEndGameEnabled)
			ImGui::EndDisabled();

		ImGui::PopStyleVar(2);
	}
	ImGui::End();
}

void UGameControlWindow::SetGameMode(AGameModeBase* InGameMode)
{
	if (InGameMode)
	{
		GameMode = TWeakPtr<AGameModeBase>(InGameMode);
	}
	else
	{
		GameMode.Reset();
	}
}

bool UGameControlWindow::IsStartButtonEnabled() const
{
	if (!GameMode.IsValid())
		return false;

	AGameModeBase* Mode = GameMode.Get();
	AGameStateBase* State = Mode->GetGameState();
	if (!State)
		return false;

	// NotStarted 상태일 때만 Start 가능
	return State->GetGameState() == EGameState::NotStarted;
}

bool UGameControlWindow::IsPauseButtonEnabled() const
{
	if (!GameMode.IsValid())
		return false;

	AGameModeBase* Mode = GameMode.Get();
	AGameStateBase* State = Mode->GetGameState();
	if (!State)
		return false;

	// Playing 상태일 때만 Pause 가능
	return State->GetGameState() == EGameState::Playing;
}

bool UGameControlWindow::IsResumeButtonEnabled() const
{
	if (!GameMode.IsValid())
		return false;

	AGameModeBase* Mode = GameMode.Get();
	AGameStateBase* State = Mode->GetGameState();
	if (!State)
		return false;

	// Paused 상태일 때만 Resume 가능
	return State->GetGameState() == EGameState::Paused;
}

bool UGameControlWindow::IsRestartButtonEnabled() const
{
	if (!GameMode.IsValid())
		return false;

	AGameModeBase* Mode = GameMode.Get();
	AGameStateBase* State = Mode->GetGameState();
	if (!State)
		return false;

	// Paused, GameOver, Victory 상태일 때 Restart 가능
	EGameState CurrentState = State->GetGameState();
	return CurrentState == EGameState::Paused ||
		   CurrentState == EGameState::GameOver ||
		   CurrentState == EGameState::Victory;
}

bool UGameControlWindow::IsEndGameButtonEnabled() const
{
	if (!GameMode.IsValid())
		return false;

	AGameModeBase* Mode = GameMode.Get();
	AGameStateBase* State = Mode->GetGameState();
	if (!State)
		return false;

	// Playing, Paused 상태일 때만 End Game 가능
	EGameState CurrentState = State->GetGameState();
	return CurrentState == EGameState::Playing ||
		   CurrentState == EGameState::Paused;
}
