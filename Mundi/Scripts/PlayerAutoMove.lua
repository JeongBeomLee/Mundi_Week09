local _ENV = ...
-- PlayerAutoMove.lua
-- 플레이어가 자동으로 앞으로 전진하는 스크립트
-- StartGame을 눌러야 시작됨

-- 설정
local ForwardSpeed = 300.0  -- 전진 속도 (units/sec)

-- BeginPlay: Actor 초기화
function BeginPlay()
    PrintToConsole("[PlayerAutoMove] Script initialized!")
    PrintToConsole("[PlayerAutoMove] Forward speed: " .. ForwardSpeed)
    PrintToConsole("[PlayerAutoMove] Press 'Start Game' to begin!")
end

-- Tick: 매 프레임 업데이트
function Tick(deltaTime)
    -- GameMode가 있고, 게임이 시작되었는지 확인
    if GameMode and GameMode:IsGameStarted() then
        -- MoveForward 호출 (값: 1.0 = 앞으로, -1.0 = 뒤로)
        MyActor:MoveForward(1.0)
    end
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
