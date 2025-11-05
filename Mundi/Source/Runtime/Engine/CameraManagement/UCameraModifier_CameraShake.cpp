#include "pch.h"
#include "UCameraModifier_CameraShake.h"

IMPLEMENT_CLASS(UCameraModifier_CameraShake)

BEGIN_PROPERTIES(UCameraModifier_CameraShake)
	// 펄린 노이즈 커브는 내부적으로 관리되므로 에디터에 노출하지 않음
END_PROPERTIES()

UCameraModifier_CameraShake::UCameraModifier_CameraShake()
{
   SetAlphaInTime(2.f);
   SetAlphaOutTime(2.f);
   SetAlpha(0.f);
   SetIsFadingIn(true);
   EnableModifier();
   SetPriority(0);

   SetNumSamples(12);

   // Curve 한 번만 생성
   GetNewPerlinNoise();
}

void UCameraModifier_CameraShake::GetNewPerlinNoise()
{
   PerlinNoiseXAxis.RenewCurve();
   PerlinNoiseYAxis.RenewCurve();
   PerlinNoiseZAxis.RenewCurve();
}

void UCameraModifier_CameraShake::ModifyCamera(
   float DeltaTime,
   FVector& InOutViewLocation,
   FQuat& InOutViewRotation,
   float& InOutFOV
)
{
   UCameraModifier::ModifyCamera(
      DeltaTime,
      InOutViewLocation,
      InOutViewRotation,
      InOutFOV
   );

   const TArray<FRichCurveKey>* XKeys = nullptr;
   const TArray<FRichCurveKey>* YKeys = nullptr;
   const TArray<FRichCurveKey>* ZKeys = nullptr;
   
   switch (ShakeCurveType)
   {
   case ECurveType::ECT_PERLIN_NOISE:
      XKeys = &PerlinNoiseXAxis.GetCurve().Keys;
      YKeys = &PerlinNoiseYAxis.GetCurve().Keys;
      ZKeys = &PerlinNoiseZAxis.GetCurve().Keys;
      break;
   case ECurveType::ECT_BEZIER:
      XKeys = &BezierCurveXAxis.GetCurve().Keys;
      YKeys = &BezierCurveYAxis.GetCurve().Keys;
      ZKeys = &BezierCurveZAxis.GetCurve().Keys;
      break;
   default:
      break;
   }

   // CurveTime: 곡선의 시간 축에서의 위치 [0, AlphaInTime]
   float CurveTime = FMath::Lerp(0.f, AlphaInTime, Alpha);
   int32 NumSamples = PerlinNoiseXAxis.GetNumSamples();

   FVector ViewNewRotation;

   for (int32 i = 0; i < NumSamples - 1; i++)
   {
      if (CurveTime >= (*XKeys)[i].Time && CurveTime <= (*XKeys)[i + 1].Time)
      {
         
         // InterpAlpha: 두 키 사이에서의 보간 비율 [0, 1]
         float InterpAlpha = FMath::GetRangePct((*XKeys)[i].Time, (*XKeys)[i + 1].Time, CurveTime);

         // 각 축별로 비선형 보간 (InterpEaseInOut으로 부드러운 흔들림)
         ViewNewRotation.X = FMath::InterpEaseInOut(
            (*XKeys)[i].Value,
            (*XKeys)[i + 1].Value,
            InterpAlpha,
            2.f
         ) * RotationAmplitude;

         ViewNewRotation.Y = FMath::InterpEaseInOut(
            (*YKeys)[i].Value,
            (*YKeys)[i + 1].Value,
            InterpAlpha,
            2.f
         ) * RotationAmplitude;

         ViewNewRotation.Z = FMath::InterpEaseInOut(
            (*ZKeys)[i].Value,
            (*ZKeys)[i + 1].Value,
            InterpAlpha,
            2.f
         ) * RotationAmplitude;
         
         // [Alternative Unreal 방식 - 주석]
         // Base Rotation에 Shake Offset을 Euler 각도로 더하는 방식:
         FVector CurrentEuler = InOutViewRotation.ToEulerZYXDeg();
         FVector NewEuler = CurrentEuler + ViewNewRotation;
         InOutViewRotation = FQuat::MakeFromEulerZYX(NewEuler);
         
         break;
      }
   }
}

float UCameraModifier_CameraShake::GetRotationAmplitude() const
{
   return RotationAmplitude;
}
void UCameraModifier_CameraShake::SetRotationAmplitude(const float InRotationAmplitude)
{
   RotationAmplitude = InRotationAmplitude;
}

// CameraShake 기간과 Curve의 TimeDuration을 동일하게
void UCameraModifier_CameraShake::SetAlphaInTime(const float InAlphaInTime)
{
   UCameraModifier::SetAlphaInTime(InAlphaInTime);

   // 펄린 노이즈 초기화
   PerlinNoiseXAxis.SetTimeRange(InAlphaInTime);
   PerlinNoiseYAxis.SetTimeRange(InAlphaInTime);
   PerlinNoiseZAxis.SetTimeRange(InAlphaInTime);

   // 커스텀 커브 초기화
   BezierCurveXAxis.SetTimeRange(InAlphaInTime);
   BezierCurveYAxis.SetTimeRange(InAlphaInTime);
   BezierCurveZAxis.SetTimeRange(InAlphaInTime);
}

void UCameraModifier_CameraShake::SetNumSamples(const float InNumSamples)
{
   // 펄린 노이즈 초기화
   PerlinNoiseXAxis.SetNumSamples(InNumSamples);
   PerlinNoiseYAxis.SetNumSamples(InNumSamples);
   PerlinNoiseZAxis.SetNumSamples(InNumSamples);

   BezierCurveXAxis.SetNumSamples(InNumSamples);
   BezierCurveYAxis.SetNumSamples(InNumSamples);
   BezierCurveZAxis.SetNumSamples(InNumSamples);
}

void UCameraModifier_CameraShake::GetPerlinNoise(
   UCurveFloat& InOutPerlinXAxis,
   UCurveFloat& InOutPerlinYAxis,
   UCurveFloat& InOutPerlinZAxis
) const
{
   InOutPerlinXAxis = PerlinNoiseXAxis;
   InOutPerlinYAxis = PerlinNoiseYAxis;
   InOutPerlinZAxis = PerlinNoiseZAxis;
}

void UCameraModifier_CameraShake::GetBezierCurve(
   UCurveFloat& InOutBezierCurveXAxis,
   UCurveFloat& InOutBezierCurveYAxis,
   UCurveFloat& InOutBezierCurveZAxis
) const
{
   InOutBezierCurveXAxis = BezierCurveXAxis;
   InOutBezierCurveYAxis = BezierCurveYAxis;
   InOutBezierCurveZAxis = BezierCurveZAxis;
}

void UCameraModifier_CameraShake::SetBezierCurve(
   const UCurveFloat& InBezierCurveXAxis,
   const UCurveFloat& InBezierCurveYAxis,
   const UCurveFloat& InBezierCurveZAxis
)
{
   BezierCurveXAxis = InBezierCurveXAxis;
   BezierCurveYAxis = InBezierCurveYAxis;
   BezierCurveZAxis = InBezierCurveZAxis;
}

ECurveType UCameraModifier_CameraShake::GetShakeCurveType() const
{
   return ShakeCurveType;
}

void UCameraModifier_CameraShake::SetShakeCurveType(const ECurveType& InShakeCurveType)
{
   ShakeCurveType = InShakeCurveType;
}