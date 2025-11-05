#pragma once

enum class ERichCurveInterpMode
{
    RCIM_Linear,
    // RCIM_Constant,
    // RCIM_Cubic,
    // RCIM_None
};

struct FRichCurveKey
{
    float Time = 0.0f;             // X축 (초 단위)
    float Value = 0.0f;            // Y축 (float 값)
    float ArriveTangent = 0.0f;    // 이전 구간에서 들어올 때 기울기
    float LeaveTangent = 0.0f;     // 다음 구간으로 나갈 때 기울기
};

struct FRichCurve
{
    TArray<FRichCurveKey> Keys;
    ERichCurveInterpMode InterpMode;
};

enum class ECurveType
{
    ECT_PERLIN_NOISE,
    ECT_BEZIER
};

// 커브 평가 유틸리티 함수
// FRichCurve에서 주어진 시간에 대한 값을 선형 보간으로 샘플링
inline float EvaluateCurve(const FRichCurve& Curve, float Time)
{
    if (Curve.Keys.Num() == 0)
    {
        return 0.0f;
    }

    // 시간이 첫 번째 키보다 이전이면 첫 번째 키의 값 반환
    if (Time <= Curve.Keys[0].Time)
    {
        return Curve.Keys[0].Value;
    }

    // 시간이 마지막 키보다 이후이면 마지막 키의 값 반환
    if (Time >= Curve.Keys.Last().Time)
    {
        return Curve.Keys.Last().Value;
    }

    // 두 키 사이를 선형 보간
    for (int32 i = 0; i < Curve.Keys.Num() - 1; ++i)
    {
        const FRichCurveKey& Key1 = Curve.Keys[i];
        const FRichCurveKey& Key2 = Curve.Keys[i + 1];

        if (Time >= Key1.Time && Time <= Key2.Time)
        {
            float Alpha = (Time - Key1.Time) / (Key2.Time - Key1.Time);
            return FMath::Lerp(Key1.Value, Key2.Value, Alpha);
        }
    }

    return 0.0f;
}