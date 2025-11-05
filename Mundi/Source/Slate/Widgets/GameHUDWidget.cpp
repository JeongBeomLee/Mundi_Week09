#include "pch.h"
#include "GameHUDWidget.h"
#include "GameStateBase.h"
#include "RunnerGameMode.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "D3D11RHI.h"
#include <sstream>
#include <iomanip>

IMPLEMENT_CLASS(UGameHUDWidget)

UGameHUDWidget::UGameHUDWidget()
	: GameState(nullptr)
	, GameMode(nullptr)
	, GameStateChangedHandle(0)
	, ScoreChangedHandle(0)
	, TimerUpdatedHandle(0)
	, PlayerHealthDecreasedHandle(0)
	, CachedScore(0)
	, CachedElapsedTime(0.0f)
	, CachedGameStateText("Not Started")
	, CachedPlayerHealth(PLAYER_HEALTH)
	, ViewportX(0.0f)
	, ViewportY(0.0f)
	, ViewportWidth(1920.0f)
	, ViewportHeight(1080.0f)
	, HeartIconTexture(nullptr)
	, HeartIconWidth(0)
	, HeartIconHeight(0)
{
}

void UGameHUDWidget::Initialize()
{
	// 하트 아이콘 로드
	LoadHeartIcon();
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

	ImGuiIO& io = ImGui::GetIO();

	// GameState 확인
	bool bIsGameOver = false;
	bool bIsVictory = false;
	if (GameState.IsValid())
	{
		EGameState State = GameState.Get()->GetGameState();
		bIsGameOver = (State == EGameState::GameOver);
		bIsVictory = (State == EGameState::Victory);
	}

	// 게임 종료 상태(GameOver 또는 Victory)일 때 뷰포트 중앙에 큰 팝업 표시
	if (bIsGameOver || bIsVictory)
	{
		// 뷰포트 영역에만 어두운 배경 오버레이
		ImGui::SetNextWindowPos(ImVec2(ViewportX, ViewportY));
		ImGui::SetNextWindowSize(ImVec2(ViewportWidth, ViewportHeight));
		ImGui::SetNextWindowBgAlpha(0.75f);

		ImGuiWindowFlags overlayFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoInputs;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
		ImGui::Begin("##GameOverOverlay", nullptr, overlayFlags);
		ImGui::End();
		ImGui::PopStyleColor();

		// 뷰포트 중앙에 팝업 창
		const float popupWidth = 500.0f;
		const float popupHeight = 400.0f;
		ImGui::SetNextWindowPos(ImVec2(ViewportX + (ViewportWidth - popupWidth) * 0.5f, ViewportY + (ViewportHeight - popupHeight) * 0.5f));
		ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight));
		ImGui::SetNextWindowBgAlpha(0.95f);

		ImGuiWindowFlags popupFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::Begin("##GameEndPopup", nullptr, popupFlags))
		{
			// 큰 폰트로 게임 종료 메시지 표시
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 20));

			ImGui::Dummy(ImVec2(0, 30)); // 상단 여백

			// 타이틀 메시지
			ImVec4 titleColor = bIsVictory ? ImVec4(0.0f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
			const char* titleText = bIsVictory ? "VICTORY!" : "GAME OVER";

			// 큰 텍스트 효과 (스케일 증가)
			ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
			ImFont* font = ImGui::GetFont();
			float originalScale = font->Scale;
			font->Scale = 2.5f;
			ImGui::PushFont(font);

			float titleWidth = ImGui::CalcTextSize(titleText).x;
			ImGui::SetCursorPosX((popupWidth - titleWidth) * 0.5f);
			ImGui::TextUnformatted(titleText);

			ImGui::PopFont();
			font->Scale = originalScale;
			ImGui::PopStyleColor();

			ImGui::Dummy(ImVec2(0, 10));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0, 10));

			// 최종 스코어 표시
			font->Scale = 1.5f;
			ImGui::PushFont(font);
			FString scoreText = "Final Score: " + std::to_string(CachedScore);
			float scoreWidth = ImGui::CalcTextSize(scoreText.c_str()).x;
			ImGui::SetCursorPosX((popupWidth - scoreWidth) * 0.5f);
			ImGui::TextUnformatted(scoreText.c_str());
			ImGui::PopFont();
			font->Scale = originalScale;

			// 최종 시간 표시
			font->Scale = 1.5f;
			ImGui::PushFont(font);
			FString timeText = "Time: " + FormatTime(CachedElapsedTime);
			float timeWidth = ImGui::CalcTextSize(timeText.c_str()).x;
			ImGui::SetCursorPosX((popupWidth - timeWidth) * 0.5f);
			ImGui::TextUnformatted(timeText.c_str());
			ImGui::PopFont();
			font->Scale = originalScale;

			ImGui::Dummy(ImVec2(0, 20));

			// 재시작 안내 메시지
			const char* restartText = "Press 'Restart Game' to play again";
			float restartWidth = ImGui::CalcTextSize(restartText).x;
			ImGui::SetCursorPosX((popupWidth - restartWidth) * 0.5f);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
			ImGui::TextUnformatted(restartText);
			ImGui::PopStyleColor();

			ImGui::PopStyleVar();
		}
		ImGui::End();
	}
	else
	{
		// 일반 HUD (화면 상단)
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

		// 하트 아이콘 렌더링 (우측 상단)
		RenderHeartIcons();
	}
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

void UGameHUDWidget::SetGameMode(ARunnerGameMode* InGameMode)
{
	// 기존 GameMode 델리게이트 해제
	if (GameMode.IsValid())
	{
		ARunnerGameMode* Mode = GameMode.Get();
		Mode->OnPlayerHealthDecreased.RemoveDynamic(PlayerHealthDecreasedHandle);
	}

	if (InGameMode)
	{
		GameMode = TWeakPtr<ARunnerGameMode>(InGameMode);

		// 체력 감소 델리게이트 바인딩
		PlayerHealthDecreasedHandle = InGameMode->OnPlayerHealthDecreased.AddDynamic(this, &UGameHUDWidget::OnPlayerHealthDecreased_Handler);
	}
	else
	{
		GameMode.Reset();
	}
}

void UGameHUDWidget::SetViewportBounds(float X, float Y, float Width, float Height)
{
	ViewportX = X;
	ViewportY = Y;
	ViewportWidth = Width;
	ViewportHeight = Height;
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

	if (GameMode.IsValid())
	{
		ARunnerGameMode* Mode = GameMode.Get();
		Mode->OnPlayerHealthDecreased.RemoveDynamic(PlayerHealthDecreasedHandle);
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

void UGameHUDWidget::OnPlayerHealthDecreased_Handler(int32 CurrentHealth)
{
	// 체력 캐시 업데이트
	CachedPlayerHealth = CurrentHealth;
	UE_LOG("GameHUDWidget: Player health changed to %d", CurrentHealth);
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

void UGameHUDWidget::LoadHeartIcon()
{
	// ResourceManager를 통해 하트 아이콘 텍스처 로드
	UResourceManager& ResMgr = UResourceManager::GetInstance();
	UTexture* HeartTexture = ResMgr.Load<UTexture>("Data/Icon/heart.png");
	UTexture* EmptyHeartTexture = ResMgr.Load<UTexture>("Data/Icon/empty_heart.png");

	if (HeartTexture && HeartTexture->GetShaderResourceView())
	{
		HeartIconTexture = (void*)HeartTexture->GetShaderResourceView();
		EmptyHeartIconTexture = (void*)EmptyHeartTexture->GetShaderResourceView();
		HeartIconWidth = HeartTexture->GetWidth();
		HeartIconHeight = HeartTexture->GetHeight();
		UE_LOG("GameHUDWidget: Heart icon loaded successfully (%dx%d)", HeartIconWidth, HeartIconHeight);
	}
	else
	{
		UE_LOG("GameHUDWidget: Failed to load heart icon from Data/Icon/heart.png");
		HeartIconTexture = nullptr;
		HeartIconWidth = 0;
		HeartIconHeight = 0;
	}
}

void UGameHUDWidget::RenderHeartIcons()
{
	if (!HeartIconTexture)
		return;

	ImGuiIO& io = ImGui::GetIO();

	// 하트 아이콘 설정
	const float HeartSize = 50.0f; // 하트 크기
	const float HeartSpacing = 8.0f; // 하트 간격
	const float Padding = 20.0f; // 화면 가장자리에서의 패딩

	// 게임 화면 영역(Viewport) 기준 우측 상단 위치 계산
	const float TotalWidth = (HeartSize + HeartSpacing) * 3 - HeartSpacing + HeartSpacing + HeartSpacing;
	const float StartX = ViewportX + ViewportWidth - TotalWidth - Padding;
	const float StartY = ViewportY + Padding;

	// 투명 배경의 윈도우 생성
	ImGui::SetNextWindowPos(ImVec2(StartX, StartY));
	ImGui::SetNextWindowSize(ImVec2(TotalWidth, HeartSize + HeartSpacing));
	ImGui::SetNextWindowBgAlpha(0.0f); // 완전 투명

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoSavedSettings;

	ImGui::Begin("##HeartIcons", nullptr, flags);

	// 하트 아이콘을 오른쪽부터 왼쪽으로 그리기
	for (int i = 0; i < PLAYER_HEALTH; ++i)
	{
		if (i > 0)
		{
			ImGui::SameLine(0.0f, HeartSpacing);
		}

		bool bShouldShow = i < CachedPlayerHealth;

		if (bShouldShow)
		{
			ImGui::Image(HeartIconTexture, ImVec2(HeartSize, HeartSize));
		}
		else
		{
			ImGui::Image(EmptyHeartIconTexture, ImVec2(HeartSize, HeartSize));
		}
	}

	ImGui::End();
}
