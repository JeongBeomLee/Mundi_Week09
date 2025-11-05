#include "pch.h"
#include "UCameraModifier_Vignetting.h"

IMPLEMENT_CLASS(UCameraModifier_Vignetting)

BEGIN_PROPERTIES(UCameraModifier_Vignetting)
    // 내부적으로 관리되므로 에디터에 노출하지 않음
END_PROPERTIES()

UCameraModifier_Vignetting::UCameraModifier_Vignetting()
    : Radius(0.8f),
    Softness(0.8f),
    VignettingColor(1.0f, 207.0f / 255.0f, 64.0f / 255.0f)  // 기본 주황색
{
    // 부모 클래스 기본값 설정
    AlphaInTime = 1.0f;
    AlphaOutTime = 1.0f;
    bDisabled = true;
}

void UCameraModifier_Vignetting::ModifyPostProcess(
    float DeltaTime,
    float& PostProcessBlendWeight,
    FPostProcessSettings& PostProcessSettings
)
{
    PostProcessSettings.VignettingRadius = Radius;
    PostProcessSettings.VignettingSoftness = Softness;
    PostProcessSettings.VignettingColor = VignettingColor;

    // Alpha 값을 블렌드 가중치로 설정 (이게 없으면 PostProcessSettings가 적용되지 않음!)
    PostProcessBlendWeight = Alpha;
}

float UCameraModifier_Vignetting::GetRadius() const
{
    return Radius;
}
void UCameraModifier_Vignetting::SetRadius(float InRadius)
{
    Radius = InRadius;
}

float UCameraModifier_Vignetting::GetSoftness() const
{
    return Softness;
}
void UCameraModifier_Vignetting::SetSoftness(float InSoftness)
{
    Softness = InSoftness;
}

FVector UCameraModifier_Vignetting::GetVignettingColor() const
{
    return VignettingColor;
}
void UCameraModifier_Vignetting::SetVignettingColor(const FVector& InVignettingColor)
{
    VignettingColor = InVignettingColor;
}