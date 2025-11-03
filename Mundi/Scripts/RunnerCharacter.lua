local _ENV = ...
-- RunnerCharacter.lua
-- Runner 게임 캐릭터 제어 (Lua 중심 설계)
-- 키 바인딩: A/D (좌우 이동), Space (점프)

-- ════════════════════════════════════════════════════════════════════════════
-- 외부 모듈
-- ════════════════════════════════════════════════════════════════════════════

local GravitySystem = require("GravitySystem")

-- ════════════════════════════════════════════════════════════════════════════
-- 설정 (에디터에서 조정 가능)
-- ════════════════════════════════════════════════════════════════════════════

local Config = {
    -- 이동 설정
    MaxWalkSpeed = 15.0,        -- 최대 이동 속도 (cm/s) - 실제 속도 결정!
    AutoForwardSpeed = 1.0,     -- 자동 전진 입력 크기 (보통 1.0 유지)
    StrafeSpeed = 1.0,          -- 좌우 이동 입력 크기 (보통 1.0 유지)
    bAutoForward = true,        -- 자동 전진 활성화

    -- 점프 및 중력 설정
    JumpZVelocity = 35.0,      -- 점프 초기 속도 (cm/s, 위로)
    GravityScale = 0.1,         -- 중력 스케일 (1.0 = 기본 중력)
    AirControl = 0.3,           -- 공중 제어력 (0.0 ~ 1.0)

    -- 입력 설정
    HorizontalInput = 0.0,      -- 현재 좌우 입력 (-1.0 ~ 1.0)
    LeftInput = 0.0,            -- A 키 입력
    RightInput = 0.0,           -- D 키 입력

    -- 프로젝타일 설정
    ProjectileSpeed = 1500.0,    -- 발사체 속도 (cm/s)
    ProjectileSpawnOffset = FVector(0, 0, 0), -- 발사 위치 오프셋 (캐릭터 위치에서 시작)
    ProjectileGravityScale = 1.0, -- 발사체 중력 스케일
    ProjectileLifespan = 5.0,   -- 발사체 생명 시간 (초)

    -- 디버그
    bDebugLog = true,
}

-- ════════════════════════════════════════════════════════════════════════════
-- 내부 변수
-- ════════════════════════════════════════════════════════════════════════════

local InputComponent = nil
local CharacterMovement = nil

-- ════════════════════════════════════════════════════════════════════════════
-- 생명주기 함수
-- ════════════════════════════════════════════════════════════════════════════

function BeginPlay()
    -- CharacterMovement 컴포넌트 가져오기 및 설정 적용
    if MyActor.GetCharacterMovement then
        CharacterMovement = MyActor:GetCharacterMovement()
        if CharacterMovement then
            CharacterMovement.MaxWalkSpeed = Config.MaxWalkSpeed
            CharacterMovement.JumpZVelocity = Config.JumpZVelocity
            CharacterMovement.GravityScale = Config.GravityScale
            CharacterMovement.AirControl = Config.AirControl
        end
    end

    -- InputComponent 가져오기 및 바인딩
    if MyActor.GetInputComponent then
        InputComponent = MyActor:GetInputComponent()
        if InputComponent then
            SetupInputBindings()
        end
    end

    -- GravitySystem 초기화 (측면 충돌 시 중력 전환)
    if GravitySystem and GravitySystem.Initialize then
        GravitySystem.Initialize(MyActor)
        PrintToConsole("[RunnerCharacter] GravitySystem initialized")
    end
end

function Tick(deltaTime)
    -- 자동 전진 처리 (Lua에서!)
   
    if Config.bAutoForward then
        -- GameMode:OnCoinCollected(3)
        ProcessAutoForward(deltaTime)
        -- PrintToConsole("[RunnerCharacter] bAutoForward in lua Tick")
    end

    -- 좌우 이동 처리
    if Config.HorizontalInput ~= 0.0 then
        ProcessHorizontalMovement(deltaTime)
    end
end

function EndPlay()
    -- 스크립트 종료 시 정리 작업
end

-- ════════════════════════════════════════════════════════════════════════════
-- 입력 바인딩
-- ════════════════════════════════════════════════════════════════════════════

function SetupInputBindings()
    if not InputComponent then
        return
    end

    -- Axis 바인딩: A/D 키 (좌우 이동)
    if InputComponent.BindAxis then
        InputComponent:BindAxis("MoveRight", string.byte('D'), 1.0, OnMoveRight)
        InputComponent:BindAxis("MoveLeft", string.byte('A'), 1.0, OnMoveLeft)
    end

    -- Action 바인딩: Space 키 (점프)
    if InputComponent.BindAction then
        InputComponent:BindAction("Jump", 0x20, OnJumpPressed, OnJumpReleased)
    end

    -- Action 바인딩: 마우스 좌클릭 (프로젝타일 던지기)
    -- 마우스 버튼: -1 = Left, -2 = Right, -3 = Middle
    if InputComponent.BindAction then
        InputComponent:BindAction("ThrowProjectile", -1, OnThrowProjectile, nil)
    end
end

-- ════════════════════════════════════════════════════════════════════════════
-- 입력 콜백
-- ════════════════════════════════════════════════════════════════════════════

function OnMoveLeft(value)
    
    Config.LeftInput = value
    Config.HorizontalInput = Config.RightInput - Config.LeftInput
end

function OnMoveRight(value)
    Config.RightInput = value
    Config.HorizontalInput = Config.RightInput - Config.LeftInput
end

function OnJumpPressed()
    if MyActor.Jump then
        MyActor:Jump()
    end
end

function OnJumpReleased()
    if MyActor.StopJumping then
        MyActor:StopJumping()
    end
end

function OnThrowProjectile()
  PrintToConsole("[RunnerCharacter] function OnThrowProjectile()!")
    ThrowProjectile()
end

-- ════════════════════════════════════════════════════════════════════════════
-- 프로젝타일 발사
-- ════════════════════════════════════════════════════════════════════════════

function ThrowProjectile()
    -- World가 없으면 리턴
    if not World then
        PrintToConsole("[RunnerCharacter] ERROR: World is nil!")
        return
    end

    -- 카메라 가져오기
    local camera = World:GetCameraActor()
    if not camera then
        PrintToConsole("[RunnerCharacter] ERROR: Camera is nil!")
        return
    end

    -- 캐릭터 위치 가져오기
    local actorLocation = MyActor:GetLocation()

    -- 카메라가 바라보는 방향으로 발사
    local fireDirection = camera:GetForward()

    -- 발사 위치 계산 (캐릭터 위치 + 오프셋)
    local offset = Config.ProjectileSpawnOffset
    local spawnLocation = FVector(
        actorLocation.X + offset.X,
        actorLocation.Y + offset.Y,
        actorLocation.Z + offset.Z
    )

    -- Transform 생성
    local spawnTransform = FTransform(spawnLocation, FQuat(), FVector(1, 1, 1))

    -- ProjectileActor 생성
    local projectile = World:SpawnProjectileActor(spawnTransform)
    if not projectile then
        PrintToConsole("[RunnerCharacter] ERROR: Failed to spawn projectile!")
        return
    end

    -- 마우스 방향으로 발사
    projectile:FireInDirection(fireDirection, Config.ProjectileSpeed)

    if Config.bDebugLog then
        PrintToConsole(string.format("[RunnerCharacter] Projectile thrown! Dir: (%.2f, %.2f, %.2f)",
            fireDirection.X, fireDirection.Y, fireDirection.Z))
    end
end

-- ════════════════════════════════════════════════════════════════════════════
-- 이동 로직 (Lua에서 구현!)
-- ════════════════════════════════════════════════════════════════════════════

function ProcessAutoForward(deltaTime)
    if not MyActor.MoveForward then
        return
    end

    MyActor:MoveForward(Config.AutoForwardSpeed)
end

function ProcessHorizontalMovement(deltaTime)
    -- 입력값이 0이면 조용히 리턴 (키를 뗐을 때)
    if Config.HorizontalInput == 0.0 then
        return
    end

    if not MyActor.GetRightDirection or not MyActor.AddMovementInput then
        return
    end

    -- 우측 방향 가져오기 (C++에서 중력 방향 고려해서 계산)
    local rightDir = MyActor:GetRightDirection()
    if not rightDir then
        return
    end

    -- 방향 벡터 생성 (AddInputVector가 방향을 정규화하기 때문에 입력값만 곱함)
    local direction = FVector(
        rightDir.X * Config.HorizontalInput,
        rightDir.Y * Config.HorizontalInput,
        rightDir.Z * Config.HorizontalInput
    )

    -- AddMovementInput 호출: (방향 벡터, 입력 크기)
    -- 중요: AddInputVector가 방향을 정규화하므로, 속도는 ScaleValue로 전달해야 함!
    MyActor:AddMovementInput(direction, Config.StrafeSpeed)
end

-- ════════════════════════════════════════════════════════════════════════════
-- 유틸리티 / 설정 함수
-- ════════════════════════════════════════════════════════════════════════════

-- 자동 전진 토글
function ToggleAutoForward()
    Config.bAutoForward = not Config.bAutoForward
    PrintToConsole("[RunnerCharacter] Auto Forward: " .. tostring(Config.bAutoForward))
end

-- 자동 전진 속도 변경
function SetAutoForwardSpeed(speed)
    Config.AutoForwardSpeed = speed
    PrintToConsole("[RunnerCharacter] Auto Forward Speed: " .. speed .. " cm/s")
end

-- 좌우 이동 속도 변경
function SetStrafeSpeed(speed)
    Config.StrafeSpeed = speed
    PrintToConsole("[RunnerCharacter] Strafe Speed: " .. speed .. " cm/s")
end

-- 중력 방향 변경 (4방향 벽면 전환용)
function SetGravityDirection(x, y, z)
    if MyActor.SetGravityDirection then
        MyActor:SetGravityDirection(x, y, z)
        PrintToConsole(string.format("[RunnerCharacter] Gravity → (%.2f, %.2f, %.2f)", x, y, z))
    end
end

-- 현재 상태 출력 (디버깅용)
function PrintStatus()
    PrintToConsole("═══════════════════════════════════════")
    PrintToConsole("[RunnerCharacter] Status:")
    PrintToConsole("  Auto Forward: " .. tostring(Config.bAutoForward))
    PrintToConsole("  Forward Speed: " .. Config.AutoForwardSpeed)
    PrintToConsole("  Strafe Speed: " .. Config.StrafeSpeed)
    PrintToConsole("  Horizontal Input: " .. Config.HorizontalInput)

    if MyActor.GetVelocity then
        local vel = MyActor:GetVelocity()
        PrintToConsole(string.format("  Velocity: (%.1f, %.1f, %.1f)", vel.X, vel.Y, vel.Z))
    end

    if MyActor.IsGrounded then
        PrintToConsole("  Grounded: " .. tostring(MyActor:IsGroundched()))
    end

    PrintToConsole("═══════════════════════════════════════")
end

-- ════════════════════════════════════════════════════════════════════════════
-- 충돌 처리
-- ════════════════════════════════════════════════════════════════════════════

function OnOverlap(OverlappedComponent, OtherActor, OtherComp, ContactPoint, PenetrationDepth)
    --PrintToConsole("[RunnerCharacter] Overlapped: " .. OtherActor:GetName():ToString())
    --if Config.bDebugLog then
    --    PrintToConsole("[RunnerCharacter] Overlapped: " .. OtherActor:GetName():ToString())
    --end

    -- TODO: 벽면 감지 및 중력 방향 전환 로직
    -- 예: OtherActor의 태그를 확인해서 "WallTrigger"면 중력 방향 변경
end

-- ════════════════════════════════════════════════════════════════════════════
-- 스크립트 로드 완료
-- ════════════════════════════════════════════════════════════════════════════

PrintToConsole("[RunnerCharacter] ✓ Lua script loaded successfully!")
