#include "pch.h"
#include "CameraControlWindow.h"
#include "PlayerCameraManager.h"
#include "PlayerController.h"
#include "UCameraModifier_CameraShake.h"
#include "CameraBlendPreset.h"
#include "CameraTypes.h"
#include "UCurveFloat.h"
#include <filesystem>

IMPLEMENT_CLASS(UCameraControlWindow)

UCameraControlWindow::UCameraControlWindow()
	: CameraManager(nullptr)
{
}

void UCameraControlWindow::Initialize()
{
	UE_LOG("UCameraControlWindow: Initialize called");

	// 프리셋 목록 초기화
	RefreshPresetList();

	UE_LOG("UCameraControlWindow: Initialize completed with %d presets", AvailablePresets.Num());
}

void UCameraControlWindow::Update()
{
	// 필요시 상태 업데이트
}

void UCameraControlWindow::RenderWidget()
{
	// PIE 모드가 아니면 렌더링하지 않음
	if (!GWorld || !GWorld->bPie)
		return;

	// CameraManager가 없으면 경고 메시지 표시
	bool bHasValidManager = CameraManager.IsValid();

	// 화면 우측 상단에 배치
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - WindowWidth - Padding, Padding), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(WindowWidth, WindowHeight), ImGuiCond_Always);

	// 윈도우 플래그
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings;

	// 반투명 배경
	ImGui::SetNextWindowBgAlpha(0.85f);

	if (ImGui::Begin("Camera Controls", nullptr, windowFlags))
	{
		if (!bHasValidManager)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
			ImGui::TextWrapped("Warning: PlayerCameraManager not found!");
			ImGui::TextWrapped("Camera controls are unavailable.");
			ImGui::PopStyleColor();
			ImGui::End();
			return;
		}

		// 탭 바 렌더링
		RenderTabBar();

		ImGui::Separator();
		ImGui::Spacing();

		// 현재 활성화된 탭의 컨텐츠 렌더링
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));

		switch (CurrentTab)
		{
		case ECameraControlTab::CameraTransition:
			RenderCameraTransitionTab();
			break;
		case ECameraControlTab::FadeInOut:
			RenderFadeInOutTab();
			break;
		case ECameraControlTab::CameraShake:
			RenderCameraShakeTab();
			break;
		case ECameraControlTab::Debug:
			RenderDebugTab();
			break;
		}

		ImGui::PopStyleVar(2);
	}
	ImGui::End();
}

void UCameraControlWindow::SetPlayerCameraManager(APlayerCameraManager* InCameraManager)
{
	if (InCameraManager)
	{
		CameraManager = TWeakPtr<APlayerCameraManager>(InCameraManager);
	}
	else
	{
		CameraManager.Reset();
	}
}

// ========== Tab Bar ==========
void UCameraControlWindow::RenderTabBar()
{
	const char* TabNames[] = { "Transition", "Fade", "Shake", "Debug" };
	const float TabWidth = (WindowWidth - 40.0f) / static_cast<float>(ECameraControlTab::COUNT);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 8));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));

	for (int i = 0; i < static_cast<int>(ECameraControlTab::COUNT); ++i)
	{
		if (i > 0)
			ImGui::SameLine();

		bool bIsSelected = (CurrentTab == static_cast<ECameraControlTab>(i));

		if (bIsSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06f, 0.53f, 0.98f, 1.0f));
		}

		if (ImGui::Button(TabNames[i], ImVec2(TabWidth, TabBarHeight)))
		{
			CurrentTab = static_cast<ECameraControlTab>(i);
		}

		if (bIsSelected)
		{
			ImGui::PopStyleColor(3);
		}
	}

	ImGui::PopStyleVar(2);
}

// ========== Camera Transition Tab ==========
void UCameraControlWindow::RenderCameraTransitionTab()
{
	if (!CameraManager.IsValid())
		return;

	APlayerCameraManager* Manager = CameraManager.Get();

	if (ImGui::CollapsingHeader("Camera Transition", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(10.0f);

		// 블렌딩 상태
		bool bIsBlending = Manager->IsBlending();
		ImGui::Text("Status: %s", bIsBlending ? "Blending" : "Idle");

		if (bIsBlending)
		{
			const FViewTargetTransitionParams& Params = Manager->GetBlendParams();

			// 진행률 표시
			float Progress = 0.0f;
			if (Params.BlendTime > 0.0f)
			{
				Progress = 1.0f - (Params.BlendTimeRemaining / Params.BlendTime);
			}
			ImGui::ProgressBar(Progress, ImVec2(-1.0f, 0.0f), "");

			// 상세 정보
			ImGui::Text("Blend Time: %.2f / %.2f", Params.BlendTime - Params.BlendTimeRemaining, Params.BlendTime);
			ImGui::Text("Remaining: %.2f", Params.BlendTimeRemaining);

			// 블렌드 함수 타입
			const char* BlendFuncName = "Unknown";
			switch (Params.BlendFunction)
			{
			case EViewTargetBlendFunction::VTBlend_Linear: BlendFuncName = "Linear"; break;
			case EViewTargetBlendFunction::VTBlend_Cubic: BlendFuncName = "Cubic"; break;
			case EViewTargetBlendFunction::VTBlend_EaseIn: BlendFuncName = "EaseIn"; break;
			case EViewTargetBlendFunction::VTBlend_EaseOut: BlendFuncName = "EaseOut"; break;
			case EViewTargetBlendFunction::VTBlend_EaseInOut: BlendFuncName = "EaseInOut"; break;
			case EViewTargetBlendFunction::VTBlend_BezierCustom: BlendFuncName = "Bezier Custom"; break;
			}
			ImGui::Text("Blend Function: %s", BlendFuncName);

			// 중단 버튼
			if (ImGui::Button("Stop Blending", ImVec2(WindowWidth - 50.0f, 25.0f)))
			{
				Manager->StopBlending();
			}
		}
		else
		{
			ImGui::TextDisabled("No active camera transition");
		}

		ImGui::Unindent(10.0f);
	}
}

// ========== Fade InOut Tab ==========
void UCameraControlWindow::RenderFadeInOutTab()
{
	if (!CameraManager.IsValid())
		return;

	APlayerCameraManager* Manager = CameraManager.Get();

	const float ContentWidth = WindowWidth - 40.0f;

	// ========== Fade Parameters ==========
	ImGui::SeparatorText("Fade Parameters");
	ImGui::Spacing();

	ImGui::Text("From Alpha (0=transparent, 1=opaque):");
	ImGui::SliderFloat("##FadeFrom", &FadeFromAlpha, 0.0f, 1.0f, "%.2f");

	ImGui::Text("To Alpha:");
	ImGui::SliderFloat("##FadeTo", &FadeToAlpha, 0.0f, 1.0f, "%.2f");

	ImGui::Text("Duration (seconds):");
	ImGui::SliderFloat("##FadeDuration", &FadeDuration, 0.1f, 10.0f, "%.1f");

	ImGui::Text("Fade Color:");
	ImGui::ColorEdit3("##FadeColor", FadeColor);

	ImGui::Spacing();
	ImGui::Spacing();

	// ========== Curve Asset Selection ==========
	ImGui::SeparatorText("Curve Asset (Optional)");
	ImGui::Spacing();

	// Curve 선택 콤보박스
	const char* CurvePreview = "Linear (Default)";
	if (SelectedFadeCurveIndex >= 0 && SelectedFadeCurveIndex < AvailablePresets.Num())
	{
		CurvePreview = AvailablePresets[SelectedFadeCurveIndex].c_str();
	}

	ImGui::Text("Fade Curve:");
	ImGui::SameLine();
	ImGui::TextDisabled("(%d presets available)", AvailablePresets.Num());

	if (ImGui::BeginCombo("##FadeCurveCombo", CurvePreview))
	{
		// Linear 옵션 (기본)
		bool bIsLinearSelected = (SelectedFadeCurveIndex == -1);
		if (ImGui::Selectable("Linear (Default)", bIsLinearSelected))
		{
			SelectedFadeCurveIndex = -1;
			// 주의: Apply 버튼을 눌러야 실제 적용됨
		}
		if (bIsLinearSelected)
			ImGui::SetItemDefaultFocus();

		// 프리셋 목록
		for (int i = 0; i < AvailablePresets.Num(); i++)
		{
			bool bIsSelected = (SelectedFadeCurveIndex == i);
			if (ImGui::Selectable(AvailablePresets[i].c_str(), bIsSelected))
			{
				SelectedFadeCurveIndex = i;
			}
			if (bIsSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	// Apply Curve 버튼
	ImGui::SameLine();
	if (ImGui::Button("Apply##ApplyCurve", ImVec2(80.0f, 0)))
	{
		if (SelectedFadeCurveIndex >= 0 && SelectedFadeCurveIndex < AvailablePresets.Num())
		{
			LoadAndApplyPreset(AvailablePresets[SelectedFadeCurveIndex]);
		}
		else
		{
			Manager->SetUseBezierFade(false);
		}
	}

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current: %s",
		Manager->IsUsingBezierFade() ? "Bezier Curve" : "Linear");

	ImGui::Spacing();
	ImGui::Spacing();

	// ========== Control Buttons ==========
	ImGui::SeparatorText("Controls");
	ImGui::Spacing();

	// Start Fade 버튼
	if (ImGui::Button("Start Camera Fade", ImVec2(ContentWidth, 35.0f)))
	{
		FLinearColor Color(FadeColor[0], FadeColor[1], FadeColor[2], 1.0f);
		Manager->StartCameraFade(FadeFromAlpha, FadeToAlpha, FadeDuration, Color);
	}

	// Stop Fade 버튼
	if (ImGui::Button("Stop Camera Fade", ImVec2(ContentWidth, 35.0f)))
	{
		Manager->StopCameraFade();
		// 주의: 커브 설정은 유지 (Stop해도 다음 Fade에서 계속 사용)
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// ========== Current State ==========
	ImGui::SeparatorText("Current State");
	ImGui::Spacing();

	ImGui::Text("Fading: %s", Manager->IsFading() ? "Yes" : "No");
	ImGui::Text("Fade Amount: %.2f", Manager->GetFadeAmount());

	FLinearColor CurrentColor = Manager->GetFadeColor();
	ImGui::Text("Fade Color: (%.2f, %.2f, %.2f)", CurrentColor.R, CurrentColor.G, CurrentColor.B);
	ImGui::Text("Fade Mode: %s", Manager->IsUsingBezierFade() ? "Bezier Curve" : "Linear");

	// ========== Curve Visualization ==========
	if (Manager->IsUsingBezierFade())
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::SeparatorText("Active Fade Curve");
		ImGui::Spacing();

		// BezierCurveFade를 가져와서 시각화
		const UCurveFloat& CurveFade = Manager->GetBezierCurveFade();
		if (CurveFade.GetNumSamples() > 0)
		{
			// UCurveFloat를 FBezierControlPoints로 근사 변환
			// (UCurveFloat는 샘플링된 키 값들이므로, 시각화를 위해 대략적인 제어점 추정)
			const FRichCurve& RichCurve = CurveFade.GetCurve();

			// 첫 번째, 마지막, 중간 키들을 사용해서 대략적인 베지어 제어점 생성
			FBezierControlPoints ApproxCurve;
			if (RichCurve.Keys.Num() >= 4)
			{
				ApproxCurve.P0 = RichCurve.Keys[0].Value;
				ApproxCurve.P1 = RichCurve.Keys[RichCurve.Keys.Num() / 3].Value;
				ApproxCurve.P2 = RichCurve.Keys[(RichCurve.Keys.Num() * 2) / 3].Value;
				ApproxCurve.P3 = RichCurve.Keys.Last().Value;
			}
			else if (RichCurve.Keys.Num() >= 2)
			{
				// 키가 2개 이상이면 선형 근사
				ApproxCurve.P0 = RichCurve.Keys[0].Value;
				ApproxCurve.P1 = RichCurve.Keys[0].Value;
				ApproxCurve.P2 = RichCurve.Keys.Last().Value;
				ApproxCurve.P3 = RichCurve.Keys.Last().Value;
			}

			// 커브 시각화 (Fade 탭용 고유 ID)
			RenderBezierCurveVisualization(ApproxCurve,
				ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
				"FadeCurveVis");

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"Visualizing approximation of %d-sample curve", CurveFade.GetNumSamples());
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No curve data available");
		}
	}
}

// ========== Camera Shake Tab ==========
void UCameraControlWindow::RenderCameraShakeTab()
{
	if (!CameraManager.IsValid())
		return;

	APlayerCameraManager* Manager = CameraManager.Get();

	const float ContentWidth = WindowWidth - 40.0f;

	// ========== Shake Parameters ==========
	ImGui::SeparatorText("Shake Parameters");
	ImGui::Spacing();

	ImGui::Text("Rotation Amplitude (degrees):");
	ImGui::SliderFloat("##ShakeAmp", &ShakeRotationAmplitude, 1.0f, 30.0f, "%.1f");

	ImGui::Text("Duration (seconds):");
	ImGui::SliderFloat("##ShakeDuration", &ShakeAlphaInTime, 0.1f, 5.0f, "%.1f");

	ImGui::Text("Num Samples:");
	ImGui::SliderInt("##ShakeSamples", &ShakeNumSamples, 2, 20);

	ImGui::Spacing();
	ImGui::Spacing();

	// ========== Curve Asset Selection (3 Axes) ==========
	ImGui::SeparatorText("Curve Assets (Per Axis)");
	ImGui::Spacing();

	// X축 커브 선택
	const char* CurvePreview_X = "Perlin Noise (Default)";
	if (SelectedShakeCurveIndex_X >= 0 && SelectedShakeCurveIndex_X < AvailablePresets.Num())
	{
		CurvePreview_X = AvailablePresets[SelectedShakeCurveIndex_X].c_str();
	}

	ImGui::Text("X-Axis Curve:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::BeginCombo("##ShakeCurveCombo_X", CurvePreview_X))
	{
		bool bIsPerlinSelected = (SelectedShakeCurveIndex_X == -1);
		if (ImGui::Selectable("Perlin Noise (Default)", bIsPerlinSelected))
		{
			SelectedShakeCurveIndex_X = -1;
		}
		if (bIsPerlinSelected)
			ImGui::SetItemDefaultFocus();

		for (int i = 0; i < AvailablePresets.Num(); i++)
		{
			bool bIsSelected = (SelectedShakeCurveIndex_X == i);
			if (ImGui::Selectable(AvailablePresets[i].c_str(), bIsSelected))
			{
				SelectedShakeCurveIndex_X = i;
			}
			if (bIsSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// Y축 커브 선택
	const char* CurvePreview_Y = "Perlin Noise (Default)";
	if (SelectedShakeCurveIndex_Y >= 0 && SelectedShakeCurveIndex_Y < AvailablePresets.Num())
	{
		CurvePreview_Y = AvailablePresets[SelectedShakeCurveIndex_Y].c_str();
	}

	ImGui::Text("Y-Axis Curve:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::BeginCombo("##ShakeCurveCombo_Y", CurvePreview_Y))
	{
		bool bIsPerlinSelected = (SelectedShakeCurveIndex_Y == -1);
		if (ImGui::Selectable("Perlin Noise (Default)", bIsPerlinSelected))
		{
			SelectedShakeCurveIndex_Y = -1;
		}
		if (bIsPerlinSelected)
			ImGui::SetItemDefaultFocus();

		for (int i = 0; i < AvailablePresets.Num(); i++)
		{
			bool bIsSelected = (SelectedShakeCurveIndex_Y == i);
			if (ImGui::Selectable(AvailablePresets[i].c_str(), bIsSelected))
			{
				SelectedShakeCurveIndex_Y = i;
			}
			if (bIsSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// Z축 커브 선택
	const char* CurvePreview_Z = "Perlin Noise (Default)";
	if (SelectedShakeCurveIndex_Z >= 0 && SelectedShakeCurveIndex_Z < AvailablePresets.Num())
	{
		CurvePreview_Z = AvailablePresets[SelectedShakeCurveIndex_Z].c_str();
	}

	ImGui::Text("Z-Axis Curve:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::BeginCombo("##ShakeCurveCombo_Z", CurvePreview_Z))
	{
		bool bIsPerlinSelected = (SelectedShakeCurveIndex_Z == -1);
		if (ImGui::Selectable("Perlin Noise (Default)", bIsPerlinSelected))
		{
			SelectedShakeCurveIndex_Z = -1;
		}
		if (bIsPerlinSelected)
			ImGui::SetItemDefaultFocus();

		for (int i = 0; i < AvailablePresets.Num(); i++)
		{
			bool bIsSelected = (SelectedShakeCurveIndex_Z == i);
			if (ImGui::Selectable(AvailablePresets[i].c_str(), bIsSelected))
			{
				SelectedShakeCurveIndex_Z = i;
			}
			if (bIsSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// ========== Curve Preview (선택된 프리셋 미리보기) ==========
	// X, Y, Z 중 하나라도 선택되어 있으면 미리보기 표시
	bool bHasAnySelection = (SelectedShakeCurveIndex_X >= 0 && SelectedShakeCurveIndex_X < AvailablePresets.Num()) ||
	                         (SelectedShakeCurveIndex_Y >= 0 && SelectedShakeCurveIndex_Y < AvailablePresets.Num()) ||
	                         (SelectedShakeCurveIndex_Z >= 0 && SelectedShakeCurveIndex_Z < AvailablePresets.Num());

	if (bHasAnySelection)
	{
		ImGui::SeparatorText("Curve Preview");
		ImGui::Spacing();

		// X축 프리셋 미리보기
		if (SelectedShakeCurveIndex_X >= 0 && SelectedShakeCurveIndex_X < AvailablePresets.Num())
		{
			FString FilePath_X = FString(PresetDirectory) + "/" + AvailablePresets[SelectedShakeCurveIndex_X] + ".json";
			FCameraBlendPreset Preset_X;
			if (FCameraBlendPreset::LoadFromFile(FilePath_X, Preset_X))
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "X-Axis Preview: %s", AvailablePresets[SelectedShakeCurveIndex_X].c_str());
				RenderBezierCurveVisualization(Preset_X.BlendParams.LocationCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"PreviewX");
				ImGui::Spacing();
			}
		}

		// Y축 프리셋 미리보기
		if (SelectedShakeCurveIndex_Y >= 0 && SelectedShakeCurveIndex_Y < AvailablePresets.Num())
		{
			FString FilePath_Y = FString(PresetDirectory) + "/" + AvailablePresets[SelectedShakeCurveIndex_Y] + ".json";
			FCameraBlendPreset Preset_Y;
			if (FCameraBlendPreset::LoadFromFile(FilePath_Y, Preset_Y))
			{
				ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Y-Axis Preview: %s", AvailablePresets[SelectedShakeCurveIndex_Y].c_str());
				RenderBezierCurveVisualization(Preset_Y.BlendParams.LocationCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"PreviewY");
				ImGui::Spacing();
			}
		}

		// Z축 프리셋 미리보기
		if (SelectedShakeCurveIndex_Z >= 0 && SelectedShakeCurveIndex_Z < AvailablePresets.Num())
		{
			FString FilePath_Z = FString(PresetDirectory) + "/" + AvailablePresets[SelectedShakeCurveIndex_Z] + ".json";
			FCameraBlendPreset Preset_Z;
			if (FCameraBlendPreset::LoadFromFile(FilePath_Z, Preset_Z))
			{
				ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "Z-Axis Preview: %s", AvailablePresets[SelectedShakeCurveIndex_Z].c_str());
				RenderBezierCurveVisualization(Preset_Z.BlendParams.LocationCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"PreviewZ");
				ImGui::Spacing();
			}
		}

		ImGui::Spacing();
	}

	// ========== Control Buttons ==========
	ImGui::SeparatorText("Controls");
	ImGui::Spacing();

	// 3개 축 모두 선택되었는지 확인
	bool bAllAxesSelected = (SelectedShakeCurveIndex_X >= 0 && SelectedShakeCurveIndex_X < AvailablePresets.Num()) &&
	                         (SelectedShakeCurveIndex_Y >= 0 && SelectedShakeCurveIndex_Y < AvailablePresets.Num()) &&
	                         (SelectedShakeCurveIndex_Z >= 0 && SelectedShakeCurveIndex_Z < AvailablePresets.Num());

	// Add Camera Shake 버튼
	if (!bAllAxesSelected)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Add Shake (Bezier)", ImVec2(ContentWidth, 35.0f)))
	{
		// 3개 축 모두 프리셋 로드
		FString FilePath_X = FString(PresetDirectory) + "/" + AvailablePresets[SelectedShakeCurveIndex_X] + ".json";
		FString FilePath_Y = FString(PresetDirectory) + "/" + AvailablePresets[SelectedShakeCurveIndex_Y] + ".json";
		FString FilePath_Z = FString(PresetDirectory) + "/" + AvailablePresets[SelectedShakeCurveIndex_Z] + ".json";

		FCameraBlendPreset Preset_X, Preset_Y, Preset_Z;
		if (FCameraBlendPreset::LoadFromFile(FilePath_X, Preset_X) &&
		    FCameraBlendPreset::LoadFromFile(FilePath_Y, Preset_Y) &&
		    FCameraBlendPreset::LoadFromFile(FilePath_Z, Preset_Z))
		{
			// 각 축의 LocationCurve를 사용
			UCurveFloat* CurveX = ConvertBezierToUCurveFloat(Preset_X.BlendParams.LocationCurve, ShakeNumSamples);
			UCurveFloat* CurveY = ConvertBezierToUCurveFloat(Preset_Y.BlendParams.LocationCurve, ShakeNumSamples);
			UCurveFloat* CurveZ = ConvertBezierToUCurveFloat(Preset_Z.BlendParams.LocationCurve, ShakeNumSamples);

			if (CurveX && CurveY && CurveZ)
			{
				UCameraModifier_CameraShake* Shake = NewObject<UCameraModifier_CameraShake>();
				Shake->SetRotationAmplitude(ShakeRotationAmplitude);
				Shake->SetAlphaInTime(ShakeAlphaInTime);
				Shake->SetNumSamples(ShakeNumSamples);
				Shake->SetBezierCurve(*CurveX, *CurveY, *CurveZ);
				Shake->SetShakeCurveType(ECurveType::ECT_BEZIER);

				Manager->AddCameraModifier(Shake);
			}
		}
	}

	if (!bAllAxesSelected)
	{
		ImGui::EndDisabled();
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f),
			"All 3 axes must have curves selected");
	}

	// Add Shake (Perlin) 버튼
	if (ImGui::Button("Add Shake (Perlin Noise)", ImVec2(ContentWidth, 35.0f)))
	{
		UCameraModifier_CameraShake* Shake = NewObject<UCameraModifier_CameraShake>();
		Shake->SetRotationAmplitude(ShakeRotationAmplitude);
		Shake->SetAlphaInTime(ShakeAlphaInTime);
		Shake->SetNumSamples(ShakeNumSamples);
		Shake->GetNewPerlinNoise();

		Manager->AddCameraModifier(Shake);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// ========== Active Modifiers ==========
	ImGui::SeparatorText("Active Modifiers");
	ImGui::Spacing();

	TArray<UCameraModifier*>& ModifierList = Manager->GetModifierList();
	int activeCount = 0;
	UCameraModifier_CameraShake* FirstActiveShake = nullptr;

	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier && !Modifier->IsDisabled())
		{
			activeCount++;
			UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(Modifier);
			if (Shake)
			{
				if (!FirstActiveShake)
				{
					FirstActiveShake = Shake;
				}

				ECurveType CurveType = Shake->GetShakeCurveType();
				const char* TypeName = (CurveType == ECurveType::ECT_PERLIN_NOISE) ? "Perlin" : "Bezier";
				ImGui::Text("  Shake (%s) - Alpha: %.2f", TypeName, Shake->GetAlpha());
			}
		}
	}
	if (activeCount == 0)
	{
		ImGui::TextDisabled("  (None)");
	}

	// ========== Running Shake Visualization (First Active Shake) ==========
	if (FirstActiveShake)
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::SeparatorText("Running Shake (Real-time)");
		ImGui::Spacing();

		ECurveType CurveType = FirstActiveShake->GetShakeCurveType();

		if (CurveType == ECurveType::ECT_BEZIER)
		{
			// 베지어 모드: 3축 커브 시각화
			UCurveFloat CurveX;
			UCurveFloat CurveY;
			UCurveFloat CurveZ;

			FirstActiveShake->GetBezierCurve(CurveX, CurveY, CurveZ);
			
			// X축 커브
			if (CurveX.GetNumSamples() > 0)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "X-Axis (Roll):");
				FBezierControlPoints ApproxCurve = ApproximateCurveFromSamples(CurveX.GetCurve());
				RenderBezierCurveVisualization(ApproxCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"ShakeBezierX");
				ImGui::Spacing();
			}

			// Y축 커브
			if (CurveY.GetNumSamples() > 0)
			{
				ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Y-Axis (Pitch):");
				FBezierControlPoints ApproxCurve = ApproximateCurveFromSamples(CurveY.GetCurve());
				RenderBezierCurveVisualization(ApproxCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"ShakeBezierY");
				ImGui::Spacing();
			}

			// Z축 커브
			if (CurveZ.GetNumSamples() > 0)
			{
				ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "Z-Axis (Yaw):");
				FBezierControlPoints ApproxCurve = ApproximateCurveFromSamples(CurveZ.GetCurve());
				RenderBezierCurveVisualization(ApproxCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"ShakeBezierZ");
				ImGui::Spacing();
			}

			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"Visualizing approximation of Bezier curves");
		}
		else if (CurveType == ECurveType::ECT_PERLIN_NOISE)
		{
			// Perlin 모드: 3축 Perlin Noise 커브 시각화
			UPerlinNoiseFloat NoiseX;
			UPerlinNoiseFloat NoiseY;
			UPerlinNoiseFloat NoiseZ;

			FirstActiveShake->GetPerlinNoise(NoiseX, NoiseY, NoiseZ);

			// X축 Perlin
			if (NoiseX.GetNumSamples() > 0)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "X-Axis (Roll) - Perlin:");
				FBezierControlPoints ApproxCurve = ApproximateCurveFromSamples(NoiseX.GetCurve());
				RenderBezierCurveVisualization(ApproxCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"ShakePerlinX");
				ImGui::Spacing();
			}

			// Y축 Perlin
			if (NoiseY.GetNumSamples() > 0)
			{
				ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Y-Axis (Pitch) - Perlin:");
				FBezierControlPoints ApproxCurve = ApproximateCurveFromSamples(NoiseY.GetCurve());
				RenderBezierCurveVisualization(ApproxCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"ShakePerlinY");
				ImGui::Spacing();
			}

			// Z축 Perlin
			if (NoiseZ.GetNumSamples() > 0)
			{
				ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "Z-Axis (Yaw) - Perlin:");
				FBezierControlPoints ApproxCurve = ApproximateCurveFromSamples(NoiseZ.GetCurve());
				RenderBezierCurveVisualization(ApproxCurve,
					ImVec2(CurveVisualizationWidth, CurveVisualizationHeight),
					"ShakePerlinZ");
				ImGui::Spacing();
			}

			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"Visualizing approximation of Perlin Noise curves");
		}
	}
}

// 프리셋 목록 새로고침
void UCameraControlWindow::RefreshPresetList()
{
	AvailablePresets.Empty();

	// 현재 작업 디렉토리 저장 (디버그용)
	std::filesystem::path CurrentWorkingDir = std::filesystem::current_path();
	DebugCurrentWorkingDir = CurrentWorkingDir.string();

	std::filesystem::path AbsolutePresetPath = std::filesystem::absolute(PresetDirectory);
	DebugAbsolutePresetPath = AbsolutePresetPath.string();

	try
	{
		bDebugDirectoryExists = std::filesystem::exists(PresetDirectory);

		if (bDebugDirectoryExists)
		{
			for (const auto& Entry : std::filesystem::directory_iterator(PresetDirectory))
			{
				if (Entry.is_regular_file() && Entry.path().extension() == ".json")
				{
					FString FileName = Entry.path().stem().string();
					AvailablePresets.Add(FileName);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		// 예외 발생 시 빈 리스트 유지
		AvailablePresets.Empty();
	}

	if (AvailablePresets.Num() > 0)
	{
		SelectedPresetIndex = 0;
	}
}

// 선택된 프리셋 로드 및 적용
void UCameraControlWindow::LoadAndApplyPreset(const FString& PresetName)
{
	if (!CameraManager.IsValid())
		return;

	FString FilePath = FString(PresetDirectory) + "/" + PresetName + ".json";

	FCameraBlendPreset Preset;
	if (FCameraBlendPreset::LoadFromFile(FilePath, Preset))
	{
		// Location 커브를 Fade에 적용
		ApplyBezierCurveToFade(Preset.BlendParams.LocationCurve);

		UE_LOG("UCameraControlWindow: Preset loaded and applied - %s", PresetName.c_str());
	}
	else
	{
		UE_LOG("UCameraControlWindow: Failed to load preset - %s", PresetName.c_str());
	}
}

// 베지어 커브를 Fade에 적용
void UCameraControlWindow::ApplyBezierCurveToFade(const FBezierControlPoints& BezierCurve)
{
	if (!CameraManager.IsValid())
		return;

	APlayerCameraManager* Manager = CameraManager.Get();

	// 베지어 커브를 UCurveFloat로 변환 (기본 샘플 수 사용)
	UCurveFloat* CurveFloat = ConvertBezierToUCurveFloat(BezierCurve);
	if (!CurveFloat)
	{
		UE_LOG("ERROR: Failed to convert bezier curve to UCurveFloat");
		return;
	}

	// PlayerCameraManager에 설정하고 활성화
	Manager->SetBezierCurveFade(*CurveFloat);
	Manager->SetUseBezierFade(true);

	UE_LOG("UCameraControlWindow: Bezier curve applied to Fade");
}

// ========== Debug Tab ==========
void UCameraControlWindow::RenderDebugTab()
{
	// ========== Preset Loading Debug ==========
	ImGui::SeparatorText("Preset Loading");
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Working Dir:");
	ImGui::TextWrapped("%s", DebugCurrentWorkingDir.c_str());

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Absolute Preset Path:");
	ImGui::TextWrapped("%s", DebugAbsolutePresetPath.c_str());

	ImGui::Spacing();
	ImGui::Text("Directory Exists: %s", bDebugDirectoryExists ? "YES" : "NO");
	ImGui::Text("Presets Found: %d", AvailablePresets.Num());

	if (AvailablePresets.Num() > 0)
	{
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Preset Names:");
		for (int i = 0; i < AvailablePresets.Num(); i++)
		{
			ImGui::Text("  %d: %s", i, AvailablePresets[i].c_str());
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// ========== Refresh Button ==========
	if (ImGui::Button("Refresh Preset List", ImVec2(WindowWidth - 40.0f, 30.0f)))
	{
		RefreshPresetList();
	}

	// ========== Camera Manager Info ==========
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::SeparatorText("Camera Manager");
	ImGui::Spacing();

	if (CameraManager.IsValid())
	{
		APlayerCameraManager* Manager = CameraManager.Get();
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Connected");
		ImGui::Text("Is Fading: %s", Manager->IsFading() ? "Yes" : "No");
		ImGui::Text("Using Bezier Fade: %s", Manager->IsUsingBezierFade() ? "Yes" : "No");
		ImGui::Text("Fade Amount: %.3f", Manager->GetFadeAmount());

		FLinearColor FadeColor = Manager->GetFadeColor();
		ImGui::Text("Fade Color: (%.2f, %.2f, %.2f, %.2f)",
			FadeColor.R, FadeColor.G, FadeColor.B, FadeColor.A);
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: Disconnected");
	}
}

// ========== Bezier Curve Visualization ==========
void UCameraControlWindow::RenderBezierCurveVisualization(const FBezierControlPoints& Curve, const ImVec2& Size, const char* UniqueID)
{
	ImVec2 CursorPos = ImGui::GetCursorScreenPos();
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	const ImVec2 Padding(CurveVisualizationPadding, CurveVisualizationPadding);

	// 배경 박스
	ImVec2 BoxMin = CursorPos;
	ImVec2 BoxMax = ImVec2(CursorPos.x + Size.x, CursorPos.y + Size.y);
	DrawList->AddRectFilled(BoxMin, BoxMax, IM_COL32(30, 30, 30, 255));
	DrawList->AddRect(BoxMin, BoxMax, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

	// 그리드 선 그리기 (10x10) - CameraBlendEditor와 동일
	for (int i = 0; i <= 10; ++i)
	{
		float T = i / 10.0f;

		// 세로 그리드
		float X = CursorPos.x + Padding.x + T * (Size.x - 2 * Padding.x);
		DrawList->AddLine(
			ImVec2(X, BoxMin.y),
			ImVec2(X, BoxMax.y),
			IM_COL32(60, 60, 60, 255),
			1.0f
		);

		// 가로 그리드
		float Y = CursorPos.y + Padding.y + T * (Size.y - 2 * Padding.y);
		DrawList->AddLine(
			ImVec2(BoxMin.x, Y),
			ImVec2(BoxMax.x, Y),
			IM_COL32(60, 60, 60, 255),
			1.0f
		);
	}

	// 축 레이블
	DrawList->AddText(ImVec2(CursorPos.x + 5, CursorPos.y + Size.y - 20),
		IM_COL32(200, 200, 200, 255), "Time (0-1)");
	DrawList->AddText(ImVec2(CursorPos.x + 5, CursorPos.y + 10),
		IM_COL32(200, 200, 200, 255), "Value (0-1)");

	// 베지어 커브의 4개 제어점을 화면 좌표로 변환
	ImVec2 P0 = BezierCurveToScreenPos(0.0f, Curve.P0, CursorPos, Size, Padding);
	ImVec2 P1 = BezierCurveToScreenPos(0.33f, Curve.P1, CursorPos, Size, Padding);
	ImVec2 P2 = BezierCurveToScreenPos(0.66f, Curve.P2, CursorPos, Size, Padding);
	ImVec2 P3 = BezierCurveToScreenPos(1.0f, Curve.P3, CursorPos, Size, Padding);

	// 베지어 커브 그리기 (cyan 색상)
	DrawList->AddBezierCubic(P0, P1, P2, P3, IM_COL32(0, 255, 255, 255), 3.0f);

	// 제어점 연결선 (점선 효과)
	DrawList->AddLine(P0, P1, IM_COL32(150, 150, 150, 150), 1.0f);
	DrawList->AddLine(P2, P3, IM_COL32(150, 150, 150, 150), 1.0f);

	// 제어점 그리기
	const float PointRadius = 5.0f;
	DrawList->AddCircleFilled(P0, PointRadius, IM_COL32(255, 100, 100, 255));
	DrawList->AddCircleFilled(P1, PointRadius, IM_COL32(255, 255, 100, 255));
	DrawList->AddCircleFilled(P2, PointRadius, IM_COL32(100, 255, 100, 255));
	DrawList->AddCircleFilled(P3, PointRadius, IM_COL32(100, 100, 255, 255));

	// 제어점 값 표시
	char Label[32];
	snprintf(Label, sizeof(Label), "P0:%.2f", Curve.P0);
	DrawList->AddText(ImVec2(P0.x + 8, P0.y - 8), IM_COL32(255, 255, 255, 255), Label);

	snprintf(Label, sizeof(Label), "P1:%.2f", Curve.P1);
	DrawList->AddText(ImVec2(P1.x + 8, P1.y - 8), IM_COL32(255, 255, 255, 255), Label);

	snprintf(Label, sizeof(Label), "P2:%.2f", Curve.P2);
	DrawList->AddText(ImVec2(P2.x + 8, P2.y - 8), IM_COL32(255, 255, 255, 255), Label);

	snprintf(Label, sizeof(Label), "P3:%.2f", Curve.P3);
	DrawList->AddText(ImVec2(P3.x + 8, P3.y - 8), IM_COL32(255, 255, 255, 255), Label);

	// 인비저블 버튼으로 영역 차지 (고유한 ID 사용)
	ImGui::SetCursorScreenPos(CursorPos);
	ImGui::InvisibleButton(UniqueID, Size);
}

ImVec2 UCameraControlWindow::BezierCurveToScreenPos(float Time, float Value, const ImVec2& Origin, const ImVec2& Size, const ImVec2& Padding) const
{
	float X = Origin.x + Padding.x + Time * (Size.x - 2 * Padding.x);

	// Y축은 반전 (화면 좌표는 위가 0, 커브는 위가 1)
	float Y = Origin.y + Padding.y + (1.0f - Value) * (Size.y - 2 * Padding.y);

	return ImVec2(X, Y);
}
