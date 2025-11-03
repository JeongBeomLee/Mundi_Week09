local _ENV = ...
-- Projectile.lua
-- 발사체 동작 관리 스크립트
-- ProjectileActor에 부착되어 사용됩니다

-- ════════════════════════════════════════════════════════════════════════════
-- 설정
-- ════════════════════════════════════════════════════════════════════════════

local Config = {
    -- 디버그
    bDebugLog = true,
}

-- ════════════════════════════════════════════════════════════════════════════
-- 내부 변수
-- ════════════════════════════════════════════════════════════════════════════

local ProjectileMovement = nil
local CurrentLifetime = 0.0

-- ════════════════════════════════════════════════════════════════════════════
-- 생명주기 함수
-- ════════════════════════════════════════════════════════════════════════════

function BeginPlay()
    if Config.bDebugLog then
        PrintToConsole("[Projectile] BeginPlay - Projectile spawned")
    end

    -- ProjectileMovement 컴포넌트 가져오기
    if MyActor.GetProjectileMovement then
        ProjectileMovement = MyActor:GetProjectileMovement()
        if ProjectileMovement and Config.bDebugLog then
            PrintToConsole("[Projectile] ProjectileMovement component found")
        end
    end
end

function Tick(deltaTime)
    CurrentLifetime = CurrentLifetime + deltaTime

    -- TODO: 여기에 커스텀 로직 추가 가능
    -- 예: 트레일 이펙트, 사운드, 호밍 등
end

function EndPlay()
    if Config.bDebugLog then
        PrintToConsole("[Projectile] EndPlay - Projectile destroyed")
    end
end

-- ════════════════════════════════════════════════════════════════════════════
-- 충돌 처리
-- ════════════════════════════════════════════════════════════════════════════

function OnOverlap(OverlappedComponent, OtherActor, OtherComp, ContactPoint, PenetrationDepth)
    -- 자기 자신은 무시
    if OtherActor == MyActor then
        return
    end

    -- 액터 이름 가져오기
    local actorName = OtherActor:GetName():ToString()

    if Config.bDebugLog then
        PrintToConsole("[Projectile] Hit: " .. actorName)
    end

    -- 장애물인지 확인 (이름에 "Obstacle" 포함)
    if string.find(actorName, "Obstacle") then
        if Config.bDebugLog then
            PrintToConsole("[Projectile] Hit Obstacle! Destroying projectile...")
        end

        -- 장애물과 충돌 시 발사체 파괴
        if MyActor.SetActorHiddenInGame then
            MyActor:SetActorHiddenInGame(true)
        end

        if MyActor.Destroy then
            MyActor:Destroy()
        end
    else
        -- 장애물이 아닌 경우 통과
        if Config.bDebugLog then
            PrintToConsole("[Projectile] Hit non-obstacle actor, passing through...")
        end
    end
end

-- 부활 작업
function Restart() end

-- ════════════════════════════════════════════════════════════════════════════
-- 유틸리티 함수
-- ════════════════════════════════════════════════════════════════════════════

-- 발사체 속성 설정 (외부에서 호출 가능)
function SetSpeed(speed)
    if ProjectileMovement then
        ProjectileMovement:SetInitialSpeed(speed)
    end
end

function SetGravity(gravity)
    if ProjectileMovement then
        ProjectileMovement:SetGravity(gravity)
    end
end

function SetLifespan(lifespan)
    if ProjectileMovement then
        ProjectileMovement:SetProjectileLifespan(lifespan)
    end
end

-- ════════════════════════════════════════════════════════════════════════════
-- 스크립트 로드 완료
-- ════════════════════════════════════════════════════════════════════════════

PrintToConsole("[Projectile] ✓ Lua script loaded successfully!")
