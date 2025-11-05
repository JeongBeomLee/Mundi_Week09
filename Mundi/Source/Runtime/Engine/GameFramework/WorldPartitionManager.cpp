#include "pch.h"
#include "WorldPartitionManager.h"
#include "PrimitiveComponent.h"
#include "Actor.h"
#include "World.h"
#include "Octree.h"
#include "BVHierarchy.h"
#include "StaticMeshActor.h"
#include "StaticMeshComponent.h"
#include "Frustum.h"
#include "Gizmo/GizmoActor.h"

IMPLEMENT_CLASS(UWorldPartitionManager)

UWorldPartitionManager::UWorldPartitionManager()
{
	//FBound WorldBounds(FVector(-50, -50, -50), FVector(50, 50, 50));
	FAABB WorldBounds(FVector(-50, -50, -50), FVector(50, 50, 50));
	SceneOctree = new FOctree(WorldBounds, 0, 8, 10);
	// BVH도 동일 월드 바운드로 초기화 (더 깊고 작은 리프 설정)
	//BVH = new FBVHierachy(FBound(), 0, 5, 1);
	BVHShared = std::make_shared<FBVHierarchy>(FAABB(), 0, 8, 1);
	BVH = BVHShared; // weak_ptr에 shared_ptr 할당
	//BVH = new FBVHierachy(FBound(), 0, 10, 3);
}

UWorldPartitionManager::~UWorldPartitionManager()
{
	if (SceneOctree)
	{
		delete SceneOctree;
		SceneOctree = nullptr;
	}
	// BVH는 shared_ptr로 자동 관리되므로 명시적 삭제 불필요
	BVHShared.reset();
	BVH.reset();
}

void UWorldPartitionManager::Clear()
{
	//ClearSceneOctree();
	ClearBVHierarchy();

	ComponentDirtyQueue.Empty();
	ComponentDirtySet.Empty();
}

// 새로 만들어진 StaticMeshComponent를 등록하는 상황에서 맥락을 분명히 드러내기 위한 API입니다.
void UWorldPartitionManager::Register(UStaticMeshComponent* Smc)
{
	MarkDirty(Smc);
}

// 새로 만들어진 Actor를 등록하는 상황에서 맥락을 분명히 드러내기 위한 API입니다.
void UWorldPartitionManager::Register(AActor* Actor)
{
	MarkDirty(Actor);
}

// BulkRegister를 사용하면 틱 budget에 걸리지 않고 1틱 안에 전부 추가됩니다.
void UWorldPartitionManager::BulkRegister(const TArray<AActor*>& Actors)
{
	if (Actors.empty()) return;
	TArray<UStaticMeshComponent*> StaticMeshComponents;
	StaticMeshComponents.Reserve(Actors.size());
	
	for (AActor* Actor : Actors)
	{
		TArray<AActor*> EditorActors = GWorld->GetEditorActors();
		auto it = std::find(EditorActors.begin(), EditorActors.end(), Actor);
		if (it != EditorActors.end())
			continue; // 에디터 액터는 포함하지 않는다.
		
		const TArray<USceneComponent*> Components = Actor->GetSceneComponents();
		for (USceneComponent* Component : Components)
		{
			if (UStaticMeshComponent* Smc = Cast<UStaticMeshComponent>(Component))
			{
				StaticMeshComponents.push_back(Smc);
				ComponentDirtySet.erase(Smc);
			}
		}
	}
	
	if (BVHShared) BVHShared->BulkUpdate(StaticMeshComponents);
}

void UWorldPartitionManager::Unregister(AActor* Actor)
{
	if (!Actor) return;

	const TArray<USceneComponent*> Components = Actor->GetSceneComponents();
	for (USceneComponent* Component : Components)
	{
		Unregister(Component);
	}
}

void UWorldPartitionManager::Unregister(USceneComponent* Component)
{
	if (!Component)
		return;

	// 종료 시 안전: BVH가 이미 삭제되었을 수 있음
	if (UStaticMeshComponent* Smc = Cast<UStaticMeshComponent>(Component))
	{
		if (BVHShared.get())
		{
			BVHShared->Remove(Smc);
		}

		ComponentDirtySet.erase(Smc);
	}
}

// World Partition에서의 액터 상태를 갱신 예약
// (신규 등록에도 사용할 수 있지만 코드 가독성을 위해 Register API 사용 권장)
void UWorldPartitionManager::MarkDirty(AActor* Actor)
{
	if (!Actor) return;

	// 기즈모는 포함하지 않는다.
	if (Cast<AGizmoActor>(Actor))
		return; 
	
	const TArray<USceneComponent*> Components = Actor->GetSceneComponents();
	for (USceneComponent* Component : Components)
	{
		if (UStaticMeshComponent* Smc = Cast<UStaticMeshComponent>(Component))
		{
			MarkDirty(Smc);
		}
	}
}

// World Partition에서의 스태틱 메시 컴포넌트 상태 갱신 예약
// (신규 등록에도 사용할 수 있지만 코드 가독성을 위해 Register API 사용 권장)
void UWorldPartitionManager::MarkDirty(UStaticMeshComponent* Smc)
{
	if (!Smc) return;
	AActor* Owner = Smc->GetOwner();
	if (!Owner) return;

	// 기즈모는 포함하지 않는다.
	if (Cast<AGizmoActor>(Owner))
		return;
	
	// second: 새로운 요소가 성공적으로 삽입되었으면 true, 이미 요소가 존재하여 삽입에 실패했으면 false
	// DirtyQueue 중복 삽입 방지 로직
	if (ComponentDirtySet.insert(Smc).second)
	{
		ComponentDirtyQueue.push(Smc);
	}
}

void UWorldPartitionManager::Update(float DeltaTime, const uint32 BudgetCount)
{
	// 프레임 히칭 방지를 위해 컴포넌트 카운트 제한
	uint32 processed = 0;
	while (processed < BudgetCount)
	{
		UStaticMeshComponent* Component = nullptr;
		if (!ComponentDirtyQueue.Dequeue(Component))
		{
			break;
		}

		if (ComponentDirtySet.erase(Component) == 0)
		{
			// 이미 처리되었거나 제거됨
			continue;
		}

		if (!Component) continue;
		if (BVHShared) BVHShared->Update(Component);

		++processed;
	}

	if (BVHShared)
	{
		BVHShared->FlushRebuild();
	}
}

//void UWorldPartitionManager::RayQueryOrdered(FRay InRay, OUT TArray<std::pair<AActor*, float>>& Candidates)
//{
//    if (SceneOctree)
//    {
//        SceneOctree->QueryRayOrdered(InRay, Candidates);
//    }
//}

void UWorldPartitionManager::RayQueryClosest(FRay InRay, OUT AActor*& OutActor, OUT float& OutBestT)
{
    OutActor = nullptr;
    //if (SceneOctree)
    //{
    //    SceneOctree->QueryRayClosest(InRay, OutActor, OutBestT);
    //}
	if (BVHShared)
	{
		BVHShared->QueryRayClosest(InRay, OutActor, OutBestT);
	}
}

void UWorldPartitionManager::FrustumQuery(FFrustum InFrustum)
{
	if (BVHShared)
	{
		BVHShared->QueryFrustum(InFrustum);
	}
}

void UWorldPartitionManager::ClearSceneOctree()
{
	if (SceneOctree)
	{
		SceneOctree->Clear();
	}
}

void UWorldPartitionManager::ClearBVHierarchy()
{
	if (BVHShared)
	{
		BVHShared->Clear();
	}
}
