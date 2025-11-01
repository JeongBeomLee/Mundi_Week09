#pragma once
#include "Widget.h"

// Forward Declarations
class AGameStateBase;

// 게임 HUD 위젯 (PIE 모드에서만 표시)
// 스코어, 타이머, 게임 상태를 화면 상단에 오버레이로 표시
class UGameHUDWidget : public UWidget
{
public:
	DECLARE_CLASS(UGameHUDWidget, UWidget)

	UGameHUDWidget();
	virtual ~UGameHUDWidget() = default;

	// UWidget 오버라이드
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void RenderWidget() override;

	// GameState 설정
	void SetGameState(AGameStateBase* InGameState);

private:
	// GameState 참조 (TWeakPtr로 안전하게 참조)
	TWeakPtr<AGameStateBase> GameState;

	// 캐시된 UI 데이터
	int32 CachedScore;
	float CachedElapsedTime;
	FString CachedGameStateText;

	// UI 헬퍼 함수
	FString FormatTime(float Seconds) const;
	FString GetGameStateText() const;
	ImVec4 GetGameStateColor() const;
};
