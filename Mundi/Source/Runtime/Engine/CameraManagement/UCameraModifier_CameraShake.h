#pragma once
#include "UCameraModifier.h"
#include "Source/Runtime/Core/Math/UPerlinNoiseFloat.h"
#include "CurveData.h"

class UCameraModifier_CameraShake : public UCameraModifier
{
    DECLARE_CLASS(UCameraModifier_CameraShake, UCameraModifier)
    GENERATED_REFLECTION_BODY()
public:
    UCameraModifier_CameraShake();
    ~UCameraModifier_CameraShake() = default;

    void ModifyCamera(
       float DeltaTime,
       FVector& InOutViewLocation,
       FQuat& InOutViewRotation,
       float& InOutFOV
   ) override;

    void GetNewPerlinNoise();

    float GetRotationAmplitude() const;
    void SetRotationAmplitude(const float InRotationAmplitude);

    void SetAlphaInTime(const float InAlphaInTime) override;
    void SetNumSamples(const float InNumSamples);

    void GetPerlinNoise(
        UCurveFloat& InOutPerlinXAxis,
        UCurveFloat& InOutPerlinYAxis,
        UCurveFloat& InOutPerlinZAxis
    ) const;
    
    void GetBezierCurve(
        UCurveFloat& InOutBezierCurveXAxis,
        UCurveFloat& InOutBezierCurveYAxis,
        UCurveFloat& InOutBezierCurveZAxis
    ) const;

    void SetBezierCurve(
        const UCurveFloat& InBezierCurveXAxis,
        const UCurveFloat& InBezierCurveYAxis,
        const UCurveFloat& InBezierCurveZAxis
    );

    ECurveType GetShakeCurveType() const;
    void SetShakeCurveType(const ECurveType& InShakeCurveType);
private:
    UPerlinNoiseFloat PerlinNoiseXAxis{};
    UPerlinNoiseFloat PerlinNoiseYAxis{};
    UPerlinNoiseFloat PerlinNoiseZAxis{};

    UCurveFloat BezierCurveXAxis{};
    UCurveFloat BezierCurveYAxis{};
    UCurveFloat BezierCurveZAxis{};

    float RotationAmplitude = 20.f;
    ECurveType ShakeCurveType = ECurveType::ECT_PERLIN_NOISE;
};
