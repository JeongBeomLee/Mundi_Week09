local _ENV = ...

local Health = 5;

-- BeginPlay: Actor가 생성되거나 레벨이 시작될 때 호출
function BeginPlay()
    -- PrintToConsole("[coin_collision Begin Play] ");
end

-- EndPlay: Actor가 제거되거나 레벨이 종료될 때 호출
function EndPlay()
    -- PrintToConsole("[coin_collision] End Play] ");
end

-- OnOverlap: 다른 Actor와 충돌했을 때 호출
function OnOverlap(
    OverlappedComponent,
    OtherActor,
    OtherComp,
    ContactPoint,
    PenetrationDepth
)
    local actorName = OtherActor:GetName():ToString()
    PrintToConsole("[coin_collision] Collided with: " .. actorName)

    if actorName == "Projectile Actor" then
        Health = Health - 1
        if Health <= 0 then
            PrintToConsole("[coin_collision] Coin collected!")
            MyActor:Destroy()
        else
            PrintToConsole("[coin_collision] Health remaining: " .. Health)
        end
    end
end

-- Tick: 매 프레임마다 호출 (dt: 델타 타임)
function Tick(dt)
    -- PrintToConsole("[coin_collision] Tick] ");
end

-- 부활 작업
function Restart() end
