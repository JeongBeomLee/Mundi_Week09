#include "pch.h"
#include "PlayerCameraManager.h"
#include "ObjectFactory.h"
#include "CameraComponent.h"
#include "CameraActor.h"
#include "UCameraModifier_CameraShake.h"

IMPLEMENT_CLASS(APlayerCameraManager)

BEGIN_PROPERTIES(APlayerCameraManager)
	// PlayerCameraManager는 스폰 UI에 노출하지 않음 + 디테일 패널에도 노출 X (PlayerController가 자동 생성)
	ADD_PROPERTY(FLinearColor, FadeColor, "Fade", false, "Fade 색상")
	ADD_PROPERTY(float, FadeAmount, "Fade", false, "현재 Fade 양")
	ADD_PROPERTY(float, FadeAlphaFrom, "Fade", false, "Fade 시작 알파")
	ADD_PROPERTY(float, FadeAlphaTo, "Fade", false, "Fade 종료 알파")
	ADD_PROPERTY(float, FadeTime, " Fade", false, "Fade 총 시간")
	ADD_PROPERTY(float, FadeTimeRemaining, "Fade", false, "Fade 남은 시간")
	ADD_PROPERTY(bool, bFading, "Fade", false, "Fade 진행 중 여부")
	ADD_PROPERTY(FName, CameraStyle, "Camera", false, "카메라 스타일 프리셋")
END_PROPERTIES()

APlayerCameraManager::APlayerCameraManager()
	: FadeColor(FLinearColor(0.f, 0.f, 0.f))
	, FadeAmount(0.0f)
	, FadeAlphaFrom(0.0f)
	, FadeAlphaTo(0.0f)
	, FadeTime(0.0f)
	, FadeTimeRemaining(0.0f)
	, bFading(false)
	, CameraStyle("Default")
{
	Name = "PlayerCameraManager";
	RootComponent = CreateDefaultSubobject<USceneComponent>("SceneComponent");
}

APlayerCameraManager::~APlayerCameraManager()
{
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	if (!ViewTarget.Target)
	{
		return;
	}

	// ViewTarget 업데이트
	UpdateViewTarget(DeltaTime);

	// Fade 효과 업데이트
	UpdateFade(DeltaTime);

	// 카메라 모디파이어 적용 (추후 구현)
	ApplyCameraModifiers(DeltaTime, ViewTarget.POV_Location, ViewTarget.POV_Rotation, ViewTarget.POV_FOV);
}

void APlayerCameraManager::SetViewTarget(AActor* NewViewTarget)
{
	ViewTarget.Target = NewViewTarget;

	// 초기 POV 설정
	if (NewViewTarget)
	{
		UCameraComponent* CameraComp = FindCameraComponent(NewViewTarget);
		if (CameraComp)
		{
			ViewTarget.POV_Location = CameraComp->GetWorldLocation();
			ViewTarget.POV_Rotation = CameraComp->GetWorldRotation();
			ViewTarget.POV_FOV = CameraComp->GetFOV();
		}
		else
		{
			// CameraComponent가 없으면 액터의 Transform 사용
			ViewTarget.POV_Location = NewViewTarget->GetActorLocation();
			ViewTarget.POV_Rotation = NewViewTarget->GetActorRotation();
			ViewTarget.POV_FOV = 60.0f; // 기본값
		}
	}
}

UCameraComponent* APlayerCameraManager::GetCameraComponentForRendering() const
{
	if (!ViewTarget.Target)
	{
		return nullptr;
	}

	return FindCameraComponent(ViewTarget.Target);
}

UCameraComponent* APlayerCameraManager::FindCameraComponent(AActor* InActor) const
{
	if (!InActor)
	{
		return nullptr;
	}

	// 1. CameraActor인 경우: 내장 CameraComponent 반환
	if (ACameraActor* CameraActor = Cast<ACameraActor>(InActor))
	{
		return CameraActor->GetCameraComponent();
	}

	// 2. Character/Pawn인 경우: OwnedComponents에서 CameraComponent 검색
	const TSet<UActorComponent*>& Components = InActor->GetOwnedComponents();
	for (UActorComponent* Component : Components)
	{
		if (UCameraComponent* CameraComp = Cast<UCameraComponent>(Component))
		{
			return CameraComp;
		}
	}

	// 3. 찾지 못함
	return nullptr;
}

void APlayerCameraManager::UpdateViewTarget(float DeltaTime)
{
	if (!ViewTarget.Target)
	{
		return;
	}

	// ViewTarget의 카메라로부터 POV 업데이트
	UCameraComponent* CameraComp = FindCameraComponent(ViewTarget.Target);
	if (CameraComp)
	{
		ViewTarget.POV_Location = CameraComp->GetWorldLocation();
		ViewTarget.POV_Rotation = CameraComp->GetWorldRotation();
		ViewTarget.POV_FOV = CameraComp->GetFOV();
	}
	else
	{
		// CameraComponent가 없으면 액터의 Transform 사용
		ViewTarget.POV_Location = ViewTarget.Target->GetActorLocation();
		ViewTarget.POV_Rotation = ViewTarget.Target->GetActorRotation();
	}
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FVector& InOutLocation, FQuat& InOutRotation, float& InOutFOV)
{
	for (UCameraModifier* Modifier : ModifierList)
	{
	    if (Modifier && !Modifier->IsDisabled())
	    {
	        Modifier->ModifyCamera(
	        	DeltaTime,
	        	InOutLocation,
	        	InOutRotation,
	        	InOutFOV
	        );

			float PPBlendWeight = 0.0f;
			Modifier->ModifyPostProcess(
				DeltaTime,
				PPBlendWeight,
				ViewTarget.PostProcessSettings);
	    }
	}
}

void APlayerCameraManager::StartCameraFade(float FromAlpha, float ToAlpha, float Duration, const FLinearColor& Color)
{
	FadeAlphaFrom = FromAlpha;
	FadeAlphaTo = ToAlpha;
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	FadeColor = Color;
	bFading = true;
}

void APlayerCameraManager::StopCameraFade()
{
	bFading = false;
	FadeTimeRemaining = 0.0f;
	FadeAmount = 0.0f;
}

void APlayerCameraManager::UpdateFade(float DeltaTime)
{
	if (!bFading)
	{
		return;
	}

	FadeTimeRemaining -= DeltaTime;

	if (FadeTimeRemaining <= 0.0f)
	{
		// Fade 완료
		FadeAmount = FadeAlphaTo;
		bFading = false;
		FadeTimeRemaining = 0.0f;
	}
	else
	{
		// 선형 보간
		float Alpha = 1.0f - (FadeTimeRemaining / FadeTime);
		FadeAmount = FadeAlphaFrom + (FadeAlphaTo - FadeAlphaFrom) * Alpha;
	}
}

void APlayerCameraManager::AddCameraModifier(UCameraModifier* NewModifier)
{
	if (!NewModifier)
	{
		return;
	}

	// 이미 리스트에 있는지 확인
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier == NewModifier)
		{
			return;
		}
	}

	// 모디파이어 추가 및 초기화
	ModifierList.Add(NewModifier);
	NewModifier->AddedToCamera(this);
}

void APlayerCameraManager::RemoveCameraModifier(UCameraModifier* ModifierToRemove)
{
	if (!ModifierToRemove)
	{
		return;
	}

	// 리스트에서 제거
	for (int32 i = 0; i < ModifierList.size(); ++i)
	{
		if (ModifierList[i] == ModifierToRemove)
		{
			ModifierList.RemoveAt(i);
			return;
		}
	}
}

void APlayerCameraManager::ResetPostProcessSettings()
{
	ViewTarget.PostProcessSettings = FPostProcessSettings();
}

void APlayerCameraManager::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
}

void APlayerCameraManager::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// ViewTarget은 복제하지 않음 (런타임에 설정됨)
	// Todo: ModifierList는 복제 필요시 추가
}
