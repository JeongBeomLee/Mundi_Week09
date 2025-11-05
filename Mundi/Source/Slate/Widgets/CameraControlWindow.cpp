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
	const char* TabNames[] = { "Transition", "Fade", "Shake" };
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
			Manager->SetUseBezierFade(false);
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
		Manager->SetUseBezierFade(false);
		SelectedFadeCurveIndex = -1;
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

	// ========== DEBUG INFO ==========
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::SeparatorText("DEBUG - Preset Loading");
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
}

// ========== Camera Shake Tab ==========
void UCameraControlWindow::RenderCameraShakeTab()
{
	if (!CameraManager.IsValid())
		return;

	APlayerCameraManager* Manager = CameraManager.Get();

	if (ImGui::CollapsingHeader("Camera Shake", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(10.0f);

		// 파라미터 입력
		ImGui::Text("Rotation Amplitude (degrees):");
		ImGui::SliderFloat("##ShakeAmp", &ShakeRotationAmplitude, 1.0f, 30.0f, "%.1f");

		ImGui::Text("Duration (seconds):");
		ImGui::SliderFloat("##ShakeDuration", &ShakeAlphaInTime, 0.1f, 5.0f, "%.1f");

		ImGui::Text("Num Samples:");
		ImGui::SliderInt("##ShakeSamples", &ShakeNumSamples, 2, 20);

		ImGui::Spacing();

		// Add Camera Shake 버튼
		if (ImGui::Button("Add Camera Shake", ImVec2(WindowWidth - 50.0f, 30.0f)))
		{
			UCameraModifier_CameraShake* Shake = NewObject<UCameraModifier_CameraShake>();
			Shake->SetRotationAmplitude(ShakeRotationAmplitude);
			Shake->SetAlphaInTime(ShakeAlphaInTime);
			Shake->SetNumSamples(ShakeNumSamples);
			Shake->GetNewPerlinNoise();

			Manager->AddCameraModifier(Shake);
		}

		// 활성 모디파이어 표시
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Active Modifiers:");

		TArray<UCameraModifier*>& ModifierList = Manager->GetModifierList();
		int activeCount = 0;
		for (UCameraModifier* Modifier : ModifierList)
		{
			if (Modifier && !Modifier->IsDisabled())
			{
				activeCount++;
				UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(Modifier);
				if (Shake)
				{
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

		ImGui::Unindent(10.0f);
	}
}

// 프리셋 목록 새로고침
void UCameraControlWindow::RefreshPresetList()
{
	AvailablePresets.Empty();

	// 현재 작업 디렉토리 저장 (디버그용)
	std::filesystem::path CurrentWorkingDir = std::filesystem::current_path();
	DebugCurrentWorkingDir = CurrentWorkingDir.string();

	FString PresetPath = "Data/CameraBlendPresets";
	std::filesystem::path AbsolutePresetPath = std::filesystem::absolute(PresetPath.c_str());
	DebugAbsolutePresetPath = AbsolutePresetPath.string();

	try
	{
		bDebugDirectoryExists = std::filesystem::exists(PresetPath.c_str());

		if (bDebugDirectoryExists)
		{
			for (const auto& Entry : std::filesystem::directory_iterator(PresetPath.c_str()))
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

	FString PresetPath = "Data/CameraBlendPresets";
	FString FilePath = PresetPath + "/" + PresetName + ".json";

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

// 베지어 커브를 CameraShake에 적용
void UCameraControlWindow::ApplyBezierCurveToShake(const FBezierControlPoints& BezierCurve)
{
	if (!CameraManager.IsValid())
		return;

	// 베지어 커브를 UCurveFloat로 변환 (기본 샘플 수 사용)
	UCurveFloat* CurveFloat = ConvertBezierToUCurveFloat(BezierCurve);
	if (!CurveFloat)
	{
		UE_LOG("ERROR: Failed to convert bezier curve to UCurveFloat");
		return;
	}

	// 새로운 CameraShake 생성 시 베지어 커브 적용
	UCameraModifier_CameraShake* Shake = NewObject<UCameraModifier_CameraShake>();
	Shake->SetRotationAmplitude(ShakeRotationAmplitude);
	Shake->SetAlphaInTime(ShakeAlphaInTime);
	Shake->SetNumSamples(ShakeNumSamples);

	// 베지어 커브를 3축에 모두 적용
	Shake->SetBezierCurve(*CurveFloat, *CurveFloat, *CurveFloat);
	Shake->SetShakeCurveType(ECurveType::ECT_BEZIER);

	APlayerCameraManager* Manager = CameraManager.Get();
	Manager->AddCameraModifier(Shake);

	UE_LOG("UCameraControlWindow: Bezier curve applied to new CameraShake");
}
