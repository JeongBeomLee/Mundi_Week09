#pragma once
#include "Object.h"
#include "WeakPtr.h"

class APlayerCameraManager;
class UCurveFloat;
struct FBezierControlPoints;
struct FCameraBlendPreset;

// PIE 모드에서 PlayerCameraManager를 제어하고 모니터링하는 UI 윈도우
class UCameraControlWindow : public UObject
{
public:
	DECLARE_CLASS(UCameraControlWindow, UObject)

	UCameraControlWindow();
	virtual ~UCameraControlWindow() override = default;

	// 초기화
	void Initialize();

	// 매 프레임 업데이트
	void Update();

	// ImGui 렌더링
	void RenderWidget();

	// PlayerCameraManager 설정
	void SetPlayerCameraManager(APlayerCameraManager* InCameraManager);

private:
	// CameraManager 참조 (약한 참조로 안전하게 관리)
	TWeakPtr<APlayerCameraManager> CameraManager;

	// === Tab Management ===
	enum class ECameraControlTab
	{
		CameraTransition,
		FadeInOut,
		CameraShake,
		COUNT
	};
	ECameraControlTab CurrentTab = ECameraControlTab::CameraTransition;
	void RenderTabBar();

	// === Camera Transition Tab ===
	void RenderCameraTransitionTab();

	// === Fade InOut Tab ===
	void RenderFadeInOutTab();

	// Fade 파라미터
	float FadeFromAlpha = 1.0f;
	float FadeToAlpha = 0.0f;
	float FadeDuration = 2.0f;
	float FadeColor[3] = { 0.0f, 0.0f, 0.0f };
	int SelectedFadeCurveIndex = -1; // -1 = Linear (no curve)

	// === CameraShake Tab ===
	void RenderCameraShakeTab();

	// CameraShake 파라미터
	float ShakeRotationAmplitude = 10.0f;
	float ShakeAlphaInTime = 2.0f;
	int ShakeNumSamples = 6;

	// 프리셋 관련
	TArray<FString> AvailablePresets;
	int SelectedPresetIndex = 0;

	// 디버그 정보
	FString DebugCurrentWorkingDir;
	FString DebugAbsolutePresetPath;
	bool bDebugDirectoryExists = false;

	// 프리셋 목록 새로고침
	void RefreshPresetList();

	// 선택된 프리셋 로드 및 적용
	void LoadAndApplyPreset(const FString& PresetName);

	// 베지어 커브를 Fade에 적용
	void ApplyBezierCurveToFade(const FBezierControlPoints& BezierCurve);

	// 베지어 커브를 CameraShake에 적용
	void ApplyBezierCurveToShake(const FBezierControlPoints& BezierCurve);

	// UI 레이아웃 상수
	static constexpr float WindowWidth = 450.0f;
	static constexpr float WindowHeight = 650.0f;
	static constexpr float Padding = 20.0f;
	static constexpr float TabBarHeight = 40.0f;
};
