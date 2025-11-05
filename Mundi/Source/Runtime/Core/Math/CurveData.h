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