#include "pch.h"
#include "CoinActor.h"
#include "StaticMeshComponent.h"
#include "CollisionComponent/BoxComponent.h"
#include "RotatingMovementComponent.h"

IMPLEMENT_CLASS(ACoinActor)

BEGIN_PROPERTIES(ACoinActor)
	MARK_AS_SPAWNABLE("코인", "플레이어가 수집할 수 있는 코인 액터입니다.")
	//ADD_PROPERTY(bool, bIsCollectible, "Coin", true, "수집 가능 여부 (true: 수집 가능, false: 수집 불가능)")
	//ADD_PROPERTY(FVector, CoinLocation, "Coin", true, "코인 위치")
END_PROPERTIES()

ACoinActor::ACoinActor()
	: StaticMeshComponent(nullptr)
	, BoxComponent(nullptr)
{
	Name = "Coin";
	// StaticMeshComponent 생성 (시각적 표현)
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	if (StaticMeshComponent)
	{
		SetMeshPath("Data/Chinese coin OBJ/chinese_coin.obj");
		StaticMeshComponent->SetOwner(this);
		RootComponent = StaticMeshComponent;
	}
	// BoxComponent 생성 (충돌 감지)
	BoxComponent = CreateDefaultSubobject<UBoxComponent>("BoxComponent");
	if (BoxComponent)
	{
		BoxComponent->SetOwner(this);
		BoxComponent->SetupAttachment(RootComponent);
		BoxComponent->SetBoxExtent(FVector(17.0f, 15.0f, 15.0f));
		// 오버랩 이벤트 활성화
		BoxComponent->bGenerateOverlapEvents = true;
	}

	RotatingComponent = CreateDefaultSubobject<URotatingMovementComponent>("RotatingMovementComponent");
	if (RotatingComponent)
	{
		RotatingComponent->SetOwner(this);
		RotatingComponent->SetRotationRate(FVector(80.0f, 0.0f, 0.0f));
	}

	SetActorRotation(FVector(0.0, 90.0, 0.0));
	SetActorScale(FVector(0.05f, 0.05f, 0.05f));
}

ACoinActor::~ACoinActor()
{
}

void ACoinActor::SetMeshPath(const FString& MeshPath)
{
	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetStaticMesh(MeshPath);
	}
}

void ACoinActor::BeginPlay()
{
	// Lua 스크립트 자동 연결 (Super::BeginPlay() 전에!)
	FLuaLocalValue LuaLocalValue;
	LuaLocalValue.MyActor = this;
	LuaLocalValue.GameMode = World ? World->GetGameMode() : nullptr;

	try
	{
		UScriptManager::GetInstance().AttachScriptTo(LuaLocalValue, "coin_collision.lua");
	}
	catch (std::exception& e)
	{
		UE_LOG("[coin_collision.lua] ERROR: Lua script attachment failed: %s", e.what());
	}

	// 이제 Super::BeginPlay() 호출 → Actor::BeginPlay()에서 Lua BeginPlay 호출됨
	Super::BeginPlay();
}
