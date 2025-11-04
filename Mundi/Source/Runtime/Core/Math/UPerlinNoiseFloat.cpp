#include "pch.h"
#include "UPerlinNoiseFloat.h"
#include <random>

IMPLEMENT_CLASS(UPerlinNoiseFloat)

BEGIN_PROPERTIES(UPerlinNoiseFloat)
	// UPerlinNoiseFloat는 UCurveFloat를 상속받으며 추가 프로퍼티가 없음
END_PROPERTIES()

UPerlinNoiseFloat::UPerlinNoiseFloat(
    const float InTimeRange,
    const FVector2D& InValueRange,
    const int32 InNumSamples
) : UCurveFloat(InTimeRange, InValueRange, InNumSamples)
{
    float Interval = InTimeRange / InNumSamples;
    if (Interval >= 1.0f)
        UE_LOG("[UPerlinNoiseFloat] Warning : Interval of PerlinNoise ");
}

void UPerlinNoiseFloat::RenewCurve()
{
    UCurveFloat::RenewCurve();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    // 펄린 노이즈의 기울기는 [-1, 1] 범위
    std::uniform_int_distribution<> dist(-1, 1);

    float GridNum = std::floor(TimeRange);

    // 격자마다 랜덤 기울기를 저장
    TArray<float> RandomSlopes(GridNum, 0.0f);
    for (int i = 0; i < GridNum; i++)
    {
        RandomSlopes[i] = dist(gen);
    }

    float StartTime = 0.0f;
    float Interval = TimeRange / NumSamples - 1;

    // 시작점과 끝점은 0으로 고정
    Curve.Keys[0].Value = 0.f;
    Curve.Keys[0].Time = 0.f;
    Curve.Keys[NumSamples - 1].Value = 0.f;
    Curve.Keys[NumSamples - 1].Time = TimeRange;

    // 펄린 노이즈 연산
    for (int i = 1; i < NumSamples - 1; i++)
    {
        Curve.Keys[i].Time = Curve.Keys[i - 1].Time + Interval;
        
        float SampleTime = Curve.Keys[i].Time;
        Curve.Keys[i].ArriveTangent = RandomSlopes[std::floor(SampleTime)];
        Curve.Keys[i].LeaveTangent = RandomSlopes[std::ceil(SampleTime)];
        float SampleFrac = SampleTime - std::trunc(SampleTime);
        
        float SampleSlope = FMath::Lerp(
            Curve.Keys[i].ArriveTangent,
            Curve.Keys[i].LeaveTangent,
            SampleFrac
        );
        
        Curve.Keys[i].Value = SampleSlope * SampleFrac;
    }
}