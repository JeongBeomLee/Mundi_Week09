#pragma once
#include "UCameraModifier.h"

/**
 * UCameraModifier_LetterBox
 *
 * 레터박스 효과를 제공하는 카메라 모디파이어
 * FPostProcessSettings의 LetterBoxSize와 LetterBoxOpacity를 사용하여
 * 시네마틱 레터박스 효과를 구현합니다.
 */
class UCameraModifier_LetterBox : public UCameraModifier
{
    DECLARE_CLASS(UCameraModifier_LetterBox, UCameraModifier)
    GENERATED_REFLECTION_BODY()

public:
    UCameraModifier_LetterBox();
    virtual ~UCameraModifier_LetterBox() = default;

    // UCameraModifier interface
    virtual void ModifyPostProcess(
        float DeltaTime,
        float& PostProcessBlendWeight,
        FPostProcessSettings& PostProcessSettings
    ) override;

    /**
     * 레터박스 효과를 시작합니다.
     * @param InSize - 레터박스 크기 (0.0 ~ 1.0, 0은 없음, 1은 최대)
     * @param InOpacity - 레터박스 불투명도 (0.0 ~ 1.0)
     * @param InFadeInTime - 페이드 인 시간 (초)
     */
    void StartLetterBox(float InSize = 0.15f, float InOpacity = 1.0f, float InFadeInTime = 1.0f);

    /**
     * 레터박스 효과를 종료합니다.
     * @param InFadeOutTime - 페이드 아웃 시간 (초)
     */
    void StopLetterBox(float InFadeOutTime = 1.0f);

    // Getters / Setters
    float GetLetterBoxSize() const { return LetterBoxSize; }
    void SetLetterBoxSize(float InSize) { LetterBoxSize = FMath::Clamp(InSize, 0.0f, 1.0f); }

    float GetLetterBoxOpacity() const { return LetterBoxOpacity; }
    void SetLetterBoxOpacity(float InOpacity) { LetterBoxOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f); }

protected:
	// 원래 레터박스 크기 및 불투명도 (모디파이어 시작 시점 저장용)
	float OriginalLetterBoxSize = 0.0f;
	float OriginalLetterBoxOpacity = 1.0f;

	// 현재 레터박스 크기 및 불투명도
    float LetterBoxSize = 0.0f;
    float LetterBoxOpacity = 1.0f;

	// 목표 레터박스 크기 및 불투명도
    float TargetLetterBoxSize = 0.0f;
    float TargetLetterBoxOpacity = 1.0f;
};
