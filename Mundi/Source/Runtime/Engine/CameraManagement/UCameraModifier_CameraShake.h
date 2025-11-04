#pragma once
#include "UCameraModifier.h"
#include "Source/Runtime/Core/Math/UPerlinNoiseFloat.h"

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

    float GetRotationAmplitude() const;
    void SetRotationAmplitude(const float InRotationAmplitude);

private:
    UPerlinNoiseFloat PerlinNoiseXAxis;
    UPerlinNoiseFloat PerlinNoiseYAxis;
    UPerlinNoiseFloat PerlinNoiseZAxis;

    float RotationAmplitude = 20.f;
};
