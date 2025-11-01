-- template.lua
-- Actor에 연결되는 기본 스크립트 템플릿
-- obj 변수를 통해 해당 Actor에 접근할 수 있습니다.

-- BeginPlay: Actor가 생성되거나 레벨이 시작될 때 호출
function BeginPlay()
    PrintToConsole("[template.lue Begin Play] ");
end

-- EndPlay: Actor가 제거되거나 레벨이 종료될 때 호출
function EndPlay()
    PrintToConsole("[template.lue End Play] ");
end

-- OnOverlap: 다른 Actor와 충돌했을 때 호출
function OnOverlap(
    OverlappedComponent,
    OtherActor,
    OtherComp,
    ContactPoint,
    PenetrationDepth
)
    PrintToConsole("[template.lue Overlap] ");
end

-- Tick: 매 프레임마다 호출 (dt: 델타 타임)
function Tick(dt)
    PrintToConsole("[template.lue Tick] ");
end
