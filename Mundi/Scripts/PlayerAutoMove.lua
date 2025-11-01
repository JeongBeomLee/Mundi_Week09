local _ENV = ...
-- PlayerAutoMove.lua
-- 플레이어가 자동으로 앞으로 전진하는 스크립트

-- 설정
local ForwardSpeed = 300.0  -- 전진 속도 (units/sec)

-- BeginPlay: Actor 초기화
function BeginPlay()
    PrintToConsole("[PlayerAutoMove] Script initialized!")
    PrintToConsole("[PlayerAutoMove] Forward speed: " .. ForwardSpeed)
end

-- Tick: 매 프레임 업데이트
function Tick(deltaTime)
    -- MoveForward 호출 (값: 1.0 = 앞으로, -1.0 = 뒤로)
    MyActor:MoveForward(1.0)

    -- 속도 출력
    local velocity = MyActor:GetVelocity()
    PrintToConsole(string.format("[PlayerAutoMove] Velocity: (%.2f, %.2f, %.2f)", velocity.X, velocity.Y, velocity.Z))
end

-- OnOverlap: 충돌 처리
function OnOverlap(
    OverlappedComponent,
    OtherActor,
    OtherComp,
    ContactPoint,
    PenetrationDepth
)
    PrintToConsole("[PlayerAutoMove] Collided with: " .. OtherActor:GetName():ToString())
end

-- EndPlay: 정리 작업
function EndPlay()
    PrintToConsole("[PlayerAutoMove] Script ended")
end

PrintToConsole("[PlayerAutoMove] Script loaded successfully!")
