// ────────────────────────────────────────────────────────────────────────────
// ShapeComponent.cpp
// 충돌 시스템의 기본 Shape 컴포넌트 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "ShapeComponent.h"
#include "Actor.h"

// UShapeComponent는 추상 클래스이므로 IMPLEMENT_ABSTRACT_CLASS 사용
IMPLEMENT_CLASS(UShapeComponent)


// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

UShapeComponent::UShapeComponent()
{
	// 기본 Shape 색상 (초록색)
	ShapeColor = FVector(0.0f, 1.0f, 0.0f);

	// Overlap 이벤트 기본 활성화
	bGenerateOverlapEvents = true;

	// 디버그 렌더링 기본 설정
	bDrawOnlyIfSelected = false;

	// 물리 차단 기본 비활성화
	bBlockComponent = false;
}

UShapeComponent::~UShapeComponent()
{
	// Overlap 정보 정리
	OverlapInfos.clear();
}

void UShapeComponent::DuplicateSubObjects()
{
	// 부모 클래스 복사 처리
	Super::DuplicateSubObjects();

	// Overlap 정보는 복사하지 않음 (런타임에 새로 생성됨)
	OverlapInfos.clear();
}

// ────────────────────────────────────────────────────────────────────────────
// Overlap 관련 함수 구현
// ────────────────────────────────────────────────────────────────────────────

/**
 * 다른 Shape 컴포넌트와 겹쳐있는지 확인합니다.
 * 기본 구현은 Bounds 기반 체크를 수행합니다.
 *
 * @param Other - 확인할 상대방 Shape 컴포넌트
 * @return 겹쳐있으면 true
 */
bool UShapeComponent::IsOverlappingComponent(const UShapeComponent* Other) const
{
	if (!Other || !bGenerateOverlapEvents || !Other->bGenerateOverlapEvents)
	{
		return false;
	}

	// Bounds 기반 빠른 체크
	FBoxSphereBounds MyBounds = GetScaledBounds();
	FBoxSphereBounds OtherBounds = Other->GetScaledBounds();

	return MyBounds.Intersects(OtherBounds);
}

/**
 * 현재 Overlap 상태를 업데이트하고 이벤트를 발생시킵니다.
 * 매 프레임 호출되어 Overlap 상태 변화를 감지합니다.
 *
 * @param OtherComponents - 확인할 다른 Shape 컴포넌트들의 배열
 */
void UShapeComponent::UpdateOverlaps(const TArray<UShapeComponent*>& OtherComponents)
{
	if (!bGenerateOverlapEvents)
	{
		return;
	}

	// 1단계: 현재 프레임에서 겹쳐있는 컴포넌트 확인
	TArray<UShapeComponent*> CurrentOverlaps;

	for (UShapeComponent* OtherComp : OtherComponents)
	{
		if (OtherComp == this)
		{
			continue; // 자기 자신은 제외
		}

		if (IsOverlappingComponent(OtherComp))
		{
			CurrentOverlaps.push_back(OtherComp);
		}
	}

	// 2단계: 새로 시작된 Overlap 감지 (Begin)
	for (UShapeComponent* OtherComp : CurrentOverlaps)
	{
		FOverlapInfo* ExistingInfo = FindOverlapInfo(OtherComp);

		if (!ExistingInfo)
		{
			// 새로운 Overlap 시작
			AActor* OtherActor = OtherComp->GetOwner();
			FVector ContactPoint = (GetScaledBounds().Origin + OtherComp->GetScaledBounds().Origin) * 0.5f;
			float PenetrationDepth = 0.0f; // 추후 정밀 계산 추가 가능

			FOverlapInfo NewInfo(OtherComp, OtherActor, ContactPoint, PenetrationDepth, false);
			OverlapInfos.push_back(NewInfo);

			// Begin Overlap 이벤트 발생
			OnComponentBeginOverlap.Broadcast(this, OtherActor, OtherComp, ContactPoint, PenetrationDepth);
		}
	}

	// 3단계: 끝난 Overlap 감지 (End)
	TArray<UShapeComponent*> OverlapsToRemove;

	for (const FOverlapInfo& Info : OverlapInfos)
	{
		bool bStillOverlapping = false;

		for (UShapeComponent* CurrentComp : CurrentOverlaps)
		{
			if (Info.OtherComponent == CurrentComp)
			{
				bStillOverlapping = true;
				break;
			}
		}

		if (!bStillOverlapping)
		{
			// Overlap 종료
			OverlapsToRemove.push_back(Info.OtherComponent);

			// End Overlap 이벤트 발생
			OnComponentEndOverlap.Broadcast(
				this,
				Info.OtherActor,
				Info.OtherComponent,
				Info.ContactPoint,
				Info.PenetrationDepth
			);
		}
	}

	// 4단계: 종료된 Overlap 정보 제거
	for (UShapeComponent* CompToRemove : OverlapsToRemove)
	{
		RemoveOverlapInfo(CompToRemove);
	}
}

/**
 * 특정 컴포넌트와의 Overlap 정보를 찾습니다.
 *
 * @param OtherComponent - 찾을 컴포넌트
 * @return Overlap 정보 포인터 (없으면 nullptr)
 */
FOverlapInfo* UShapeComponent::FindOverlapInfo(const UShapeComponent* OtherComponent)
{
	for (FOverlapInfo& Info : OverlapInfos)
	{
		if (Info.OtherComponent == OtherComponent)
		{
			return &Info;
		}
	}

	return nullptr;
}

/**
 * 특정 컴포넌트와의 Overlap 정보를 제거합니다.
 *
 * @param OtherComponent - 제거할 컴포넌트
 * @return 제거 성공 여부
 */
bool UShapeComponent::RemoveOverlapInfo(const UShapeComponent* OtherComponent)
{
	for (auto It = OverlapInfos.begin(); It != OverlapInfos.end(); ++It)
	{
		if (It->OtherComponent == OtherComponent)
		{
			OverlapInfos.erase(It);
			return true;
		}
	}

	return false;
}

// ────────────────────────────────────────────────────────────────────────────
// 디버그 렌더링
// ────────────────────────────────────────────────────────────────────────────

/**
 * Renderer를 통해 디버그 볼륨을 렌더링합니다.
 * 기본 구현은 아무것도 하지 않으며, 자식 클래스에서 오버라이드합니다.
 *
 * @param Renderer - 렌더러 객체
 */
void UShapeComponent::RenderDebugVolume(URenderer* Renderer) const
{
	// 기본 구현: 아무것도 렌더링하지 않음
	// 자식 클래스(BoxComponent, SphereComponent 등)에서 오버라이드
}
