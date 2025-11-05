#include "pch.h"
#include "CameraTypes.h"

// 큐빅 베지어 평가 함수
// B(t) = (1-t)³*P0 + 3(1-t)²t*P1 + 3(1-t)t²*P2 + t³*P3
float FBezierControlPoints::Evaluate(float T) const
{
	// T를 0~1 범위로 클램프
	T = FMath::Clamp(T, 0.0f, 1.0f);

	// 베지어 계수 계산
	float U = 1.0f - T;
	float W0 = U * U * U;              // (1-t)³
	float W1 = 3.0f * U * U * T;       // 3(1-t)²t
	float W2 = 3.0f * U * T * T;       // 3(1-t)t²
	float W3 = T * T * T;              // t³

	// 베지어 커브 평가
	return W0 * P0 + W1 * P1 + W2 * P2 + W3 * P3;
}

// 선형 커브 (0, 0.33, 0.66, 1)
FBezierControlPoints FBezierControlPoints::Linear()
{
	return FBezierControlPoints(0.0f, 0.33f, 0.66f, 1.0f);
}

// EaseIn 커브 (천천히 시작)
FBezierControlPoints FBezierControlPoints::EaseIn()
{
	return FBezierControlPoints(0.0f, 0.0f, 0.58f, 1.0f);
}

// EaseOut 커브 (천천히 종료)
FBezierControlPoints FBezierControlPoints::EaseOut()
{
	return FBezierControlPoints(0.0f, 0.42f, 1.0f, 1.0f);
}

// EaseInOut 커브 (천천히 시작하고 종료)
FBezierControlPoints FBezierControlPoints::EaseInOut()
{
	return FBezierControlPoints(0.0f, 0.0f, 1.0f, 1.0f);
}

// Cubic 커브 (부드러운 가속/감속)
FBezierControlPoints FBezierControlPoints::Cubic()
{
	return FBezierControlPoints(0.0f, 0.25f, 0.75f, 1.0f);
}

// 블렌드 알파 계산
float FViewTargetTransitionParams::CalculateBlendAlpha() const
{
	if (BlendTime <= 0.0f)
		return 1.0f;

	// 진행률 계산 (0~1)
	float TimeRatio = 1.0f - (BlendTimeRemaining / BlendTime);
	TimeRatio = FMath::Clamp(TimeRatio, 0.0f, 1.0f);

	// 블렌드 함수에 따라 알파 계산
	float Alpha = 0.0f;

	switch (BlendFunction)
	{
	case EViewTargetBlendFunction::VTBlend_Linear:
		Alpha = TimeRatio;
		break;

	case EViewTargetBlendFunction::VTBlend_Cubic:
		// 큐빅 보간 (부드러운 가속/감속)
		Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, TimeRatio, 2.0f);
		break;

	case EViewTargetBlendFunction::VTBlend_EaseIn:
		// 천천히 시작
		Alpha = pow(TimeRatio, BlendExp);
		break;

	case EViewTargetBlendFunction::VTBlend_EaseOut:
		// 천천히 종료
		Alpha = 1.0f - pow(1.0f - TimeRatio, BlendExp);
		break;

	case EViewTargetBlendFunction::VTBlend_EaseInOut:
		// 천천히 시작하고 종료
		Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, TimeRatio, BlendExp);
		break;

	case EViewTargetBlendFunction::VTBlend_BezierCustom:
		// 커스텀 베지어는 각 채널별로 다르게 계산
		// 기본적으로 LocationCurve 사용
		Alpha = LocationCurve.Evaluate(TimeRatio);
		break;

	default:
		Alpha = TimeRatio;
		break;
	}

	return FMath::Clamp(Alpha, 0.0f, 1.0f);
}

// 위치 블렌딩 알파 계산
float FViewTargetTransitionParams::CalculateLocationAlpha() const
{
	if (BlendTime <= 0.0f)
		return 1.0f;

	float TimeRatio = 1.0f - (BlendTimeRemaining / BlendTime);
	TimeRatio = FMath::Clamp(TimeRatio, 0.0f, 1.0f);

	if (BlendFunction == EViewTargetBlendFunction::VTBlend_BezierCustom)
	{
		return LocationCurve.Evaluate(TimeRatio);
	}

	return CalculateBlendAlpha();
}

// 회전 블렌딩 알파 계산
float FViewTargetTransitionParams::CalculateRotationAlpha() const
{
	if (BlendTime <= 0.0f)
		return 1.0f;

	float TimeRatio = 1.0f - (BlendTimeRemaining / BlendTime);
	TimeRatio = FMath::Clamp(TimeRatio, 0.0f, 1.0f);

	if (BlendFunction == EViewTargetBlendFunction::VTBlend_BezierCustom)
	{
		return RotationCurve.Evaluate(TimeRatio);
	}

	return CalculateBlendAlpha();
}

// FOV 블렌딩 알파 계산
float FViewTargetTransitionParams::CalculateFOVAlpha() const
{
	if (BlendTime <= 0.0f)
		return 1.0f;

	float TimeRatio = 1.0f - (BlendTimeRemaining / BlendTime);
	TimeRatio = FMath::Clamp(TimeRatio, 0.0f, 1.0f);

	if (BlendFunction == EViewTargetBlendFunction::VTBlend_BezierCustom)
	{
		return FOVCurve.Evaluate(TimeRatio);
	}

	return CalculateBlendAlpha();
}

// SpringArm 블렌딩 알파 계산
float FViewTargetTransitionParams::CalculateSpringArmAlpha() const
{
	if (BlendTime <= 0.0f)
		return 1.0f;

	float TimeRatio = 1.0f - (BlendTimeRemaining / BlendTime);
	TimeRatio = FMath::Clamp(TimeRatio, 0.0f, 1.0f);

	if (BlendFunction == EViewTargetBlendFunction::VTBlend_BezierCustom)
	{
		return SpringArmCurve.Evaluate(TimeRatio);
	}

	return CalculateBlendAlpha();
}

// 블렌드 업데이트
void FViewTargetTransitionParams::UpdateBlend(float DeltaTime)
{
	if (BlendTimeRemaining > 0.0f)
	{
		BlendTimeRemaining -= DeltaTime;
		if (BlendTimeRemaining < 0.0f)
		{
			BlendTimeRemaining = 0.0f;
		}
	}
}
