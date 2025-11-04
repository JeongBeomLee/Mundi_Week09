#pragma once
#include "Actor.h"
#include "ViewTarget.h"
#include "Color.h"

class UCameraComponent;
class UCameraModifier; // 전방 선언 (다른 팀원이 구현 중)

// 플레이어 카메라를 관리하는 매니저 클래스
// PlayerController가 소유하며, ViewTarget의 카메라를 렌더링에 제공
class APlayerCameraManager : public AActor
{
public:
	DECLARE_CLASS(APlayerCameraManager, AActor)
	GENERATED_REFLECTION_BODY()

	APlayerCameraManager();

protected:
	~APlayerCameraManager() override;

public:
	// 매 프레임 카메라 업데이트 (PlayerController::Tick에서 호출)
	virtual void UpdateCamera(float DeltaTime);

	// ViewTarget 설정 (현재 렌더링할 액터, 일반적으로 플레이어의 Pawn)
	void SetViewTarget(AActor* NewViewTarget);
	AActor* GetViewTarget() const { return ViewTarget.Target; }

	// 렌더링용 카메라 컴포넌트 반환
	UCameraComponent* GetCameraComponentForRendering() const;

	// ViewTarget POV 접근자
	const FViewTarget& GetViewTarget_Internal() const { return ViewTarget; }
	FVector GetCameraLocation() const { return ViewTarget.POV_Location; }
	FQuat GetCameraRotation() const { return ViewTarget.POV_Rotation; }
	float GetCameraFOV() const { return ViewTarget.POV_FOV; }

	// 카메라 스타일 설정 (프리셋)
	void SetCameraStyle(const FName& NewStyle) { CameraStyle = NewStyle; }
	FName GetCameraStyle() const { return CameraStyle; }

	// Fade 효과 (구조만 구현, 실제 렌더링 연동은 추후)
	void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, const FLinearColor& Color);
	void StopCameraFade();

	// 카메라 모디파이어 관리
	void AddCameraModifier(UCameraModifier* NewModifier);
	void RemoveCameraModifier(UCameraModifier* ModifierToRemove);

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(APlayerCameraManager)

protected:
	// ViewTarget 업데이트
	virtual void UpdateViewTarget(float DeltaTime);

	// Fade 효과 업데이트
	void UpdateFade(float DeltaTime);

	// 카메라 모디파이어 적용 (추후 팀원 구현과 통합)
	void ApplyCameraModifiers(float DeltaTime, FVector& InOutLocation, FQuat& InOutRotation, float& InOutFOV);

	// ViewTarget으로부터 카메라 컴포넌트를 찾아서 반환
	UCameraComponent* FindCameraComponent(AActor* InActor) const;

protected:
	// Fade 효과 관련 변수
	FLinearColor FadeColor;
	float FadeAmount;
	float FadeAlphaFrom; // Fade 시작 알파
	float FadeAlphaTo;   // Fade 종료 알파
	float FadeTime;
	float FadeTimeRemaining;
	bool bFading;

	// 카메라 스타일 (프리셋)
	FName CameraStyle;

	// ViewTarget (현재 렌더링 대상)
	FViewTarget ViewTarget;

	// 카메라 모디파이어 리스트 (다른 팀원이 구현 중)
	TArray<UCameraModifier*> ModifierList;
};
