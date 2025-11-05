#include "pch.h"
#include "UCameraModifier_LetterBox.h"

IMPLEMENT_CLASS(UCameraModifier_LetterBox)

BEGIN_PROPERTIES(UCameraModifier_LetterBox)
    // 내부적으로 관리되므로 에디터에 노출하지 않음
END_PROPERTIES()

UCameraModifier_LetterBox::UCameraModifier_LetterBox()
    : LetterBoxSize(0.0f)
    , LetterBoxOpacity(1.0f)
    , TargetLetterBoxSize(0.0f)
    , TargetLetterBoxOpacity(1.0f)
{
    // 기본값 설정
    AlphaInTime = 1.0f;
    AlphaOutTime = 1.0f;
    bDisabled = true;
}

void UCameraModifier_LetterBox::ModifyPostProcess(
    float DeltaTime,
    float& PostProcessBlendWeight,
    FPostProcessSettings& PostProcessSettings
)
{
    if (bDisabled)
    {
        return;
    }

    // Alpha 값에 따라 레터박스 크기와 불투명도를 보간
    LetterBoxSize = FMath::Lerp(LetterBoxSize, TargetLetterBoxSize, Alpha);
    LetterBoxOpacity = FMath::Lerp(LetterBoxOpacity, TargetLetterBoxOpacity, Alpha);

    // PostProcessSettings에 레터박스 설정 적용
    PostProcessSettings.LetterBoxSize = LetterBoxSize;
    PostProcessSettings.LetterBoxOpacity = LetterBoxOpacity;

    // Alpha 값을 블렌드 가중치로 설정
    PostProcessBlendWeight = Alpha;
}

void UCameraModifier_LetterBox::StartLetterBox(float InSize, float InOpacity, float InFadeInTime)
{
    // 목표 값 설정
    TargetLetterBoxSize = FMath::Clamp(InSize, 0.0f, 1.0f);
    TargetLetterBoxOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);

    // 페이드 인 시간 설정
    SetAlphaInTime(InFadeInTime);

    // 모디파이어 활성화
    EnableModifier();
    SetIsFadingIn(true);
    SetAlpha(0.0f);
}

void UCameraModifier_LetterBox::StopLetterBox(float InFadeOutTime)
{
    // 목표 값을 0으로 설정
    TargetLetterBoxSize = 0.0f;
    TargetLetterBoxOpacity = 0.0f;

    // 페이드 아웃 시간 설정
    SetAlphaOutTime(InFadeOutTime);

    // 페이드 아웃 시작
    SetIsFadingIn(false);
    SetAlpha(1.0f);
}