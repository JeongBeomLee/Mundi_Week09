local _ENV = ...
-- RunnerCharacter.lua
-- Runner 게임 캐릭터 제어 (Lua 중심 설계)
-- 키 바인딩: A/D (좌우 이동), Space (점프)

-- ════════════════════════════════════════════════════════════════════════════
-- 설정 (에디터에서 조정 가능)
-- ════════════════════════════════════════════════════════════════════════════

local Config = {
    -- 이동 설정
    MaxWalkSpeed = 30.0,        -- 최대 이동 속도 (cm/s) - 실제 속도 결정!
    AutoForwardSpeed = 1.0,     -- 자동 전진 입력 크기 (보통 1.0 유지)
    StrafeSpeed = 0.1,          -- 좌우 이동 입력 크기 (보통 1.0 유지)
    bAutoForward = true,        -- 자동 전진 활성화

    -- 점프 및 중력 설정
    JumpZVelocity = 300.0,      -- 점프 초기 속도 (cm/s, 위로)
    GravityScale = 1.0,         -- 중력 스케일 (1.0 = 기본 중력)
    AirControl = 0.3,           -- 공중 제어력 (0.0 ~ 1.0)

    -- 입력 설정
    HorizontalInput = 0.0,      -- 현재 좌우 입력 (-1.0 ~ 1.0)

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
    if Config.bDebugLog then
        PrintToConsole("═══════════════════════════════════════")
        PrintToConsole("[RunnerCharacter] Lua Script Initialized!")
        PrintToConsole("═══════════════════════════════════════")
        PrintToConsole("Controls:")
        PrintToConsole("  A/D     - Strafe Left/Right (" .. Config.StrafeSpeed .. " cm/s)")
        PrintToConsole("  Space   - Jump")
        PrintToConsole("Settings:")
        PrintToConsole("  Auto Forward: " .. tostring(Config.bAutoForward))
        PrintToConsole("  Forward Speed: " .. Config.AutoForwardSpeed .. " cm/s")
        PrintToConsole("═══════════════════════════════════════")
    end

    -- CharacterMovement 컴포넌트 가져오기 및 설정 적용
    if MyActor.GetCharacterMovement then
        CharacterMovement = MyActor:GetCharacterMovement()
        if CharacterMovement then
            PrintToConsole("[RunnerCharacter] CharacterMovement component found")

            -- Lua Config 값으로 모든 설정 적용
            local success, err = pcall(function()
                PrintToConsole("[DEBUG] Setting MaxWalkSpeed to " .. Config.MaxWalkSpeed)
                CharacterMovement.MaxWalkSpeed = Config.MaxWalkSpeed

                PrintToConsole("[DEBUG] Setting JumpZVelocity to " .. Config.JumpZVelocity)
                CharacterMovement.JumpZVelocity = Config.JumpZVelocity

                PrintToConsole("[DEBUG] Setting GravityScale to " .. Config.GravityScale)
                CharacterMovement.GravityScale = Config.GravityScale

                PrintToConsole("[DEBUG] Setting AirControl to " .. Config.AirControl)
                CharacterMovement.AirControl = Config.AirControl
            end)

            if success then
                PrintToConsole("[RunnerCharacter] Movement settings applied successfully!")
            else
                PrintToConsole("[RunnerCharacter] ERROR applying settings: " .. tostring(err))
            end
        end
    end

    -- InputComponent 가져오기
    PrintToConsole("[RunnerCharacter] Trying to get InputComponent...")
    if MyActor.GetInputComponent then
        InputComponent = MyActor:GetInputComponent()
        if InputComponent then
            PrintToConsole("[RunnerCharacter] InputComponent found! Calling SetupInputBindings...")
            SetupInputBindings()
            PrintToConsole("[RunnerCharacter] SetupInputBindings completed!")
        else
            PrintToConsole("[RunnerCharacter] ERROR: InputComponent is nil!")
        end
    else
        PrintToConsole("[RunnerCharacter] ERROR: GetInputComponent method not found!")
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
    if Config.bDebugLog then
        PrintToConsole("[RunnerCharacter] Script ended")
    end
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
        -- D 키 = 오른쪽 (+1.0)
        InputComponent:BindAxis("MoveRight", string.byte('D'), 1.0, OnMoveHorizontal)
        -- A 키 = 왼쪽 (-1.0)
        InputComponent:BindAxis("MoveRight", string.byte('A'), -1.0, OnMoveHorizontal)

        if Config.bDebugLog then
            PrintToConsole("[RunnerCharacter] ✓ Bound A/D keys to MoveRight axis")
        end
    end

    -- Action 바인딩: Space 키 (점프)
    if InputComponent.BindAction then
        -- VK_SPACE = 0x20 = 32
        InputComponent:BindAction("Jump", 0x20, OnJumpPressed, OnJumpReleased)
        if Config.bDebugLog then
            PrintToConsole("[RunnerCharacter] ✓ Bound Space key to Jump action")
        end
    end
end

-- ════════════════════════════════════════════════════════════════════════════
-- 입력 콜백
-- ════════════════════════════════════════════════════════════════════════════

function OnMoveHorizontal(value)
    Config.HorizontalInput = value
    if value ~= 0.0 then
        PrintToConsole(string.format("[Input] MoveRight value: %.2f", value))
    end
end

function OnJumpPressed()
    if MyActor.Jump then
       local success = MyActor:Jump()
        if Config.bDebugLog and success then
            PrintToConsole("[RunnerCharacter] 🦘 Jump!")
        end
    end
end

function OnJumpReleased()
    if MyActor.StopJumping then
        MyActor:StopJumping()
    end
end

-- ════════════════════════════════════════════════════════════════════════════
-- 이동 로직 (Lua에서 구현!)
-- ════════════════════════════════════════════════════════════════════════════

function ProcessAutoForward(deltaTime)
    if not MyActor.GetForwardDirection or not MyActor.AddMovementInput then
      --  PrintToConsole("[RunnerCharacter] ERROR: Missing methods")
        return
    end
    
    -- 전진 방향 가져오기 (C++에서 제공하는 유틸리티 함수)
    local forwardDir = MyActor:GetForwardDirection()

    if not forwardDir then
    --    PrintToConsole("[RunnerCharacter] ERROR: forwardDir is nil")
        return
    end
  
    -- 정규화된 입력 값 계산 (속도 / 최대속도)
    local maxWalkSpeed = 500.0
    if CharacterMovement and CharacterMovement.MaxWalkSpeed then
        maxWalkSpeed = CharacterMovement.MaxWalkSpeed
    end
       
    local inputScale = Config.AutoForwardSpeed / maxWalkSpeed

 --  PrintToConsole("MoveForward")
    -- AddMovementInput 호출: (FVector, float)
    MyActor:MoveForward(Config.AutoForwardSpeed)
   -- PrintToConsole(string.format("[Tick] AutoForwardSpeed: (%.2f)", Config.AutoForwardSpeed))
    -- PrintToConsole(string.format("[Tick] Forward: (%.2f, %.2f, %.2f)", scaledDir.X, scaledDir.Y, scaledDir.Z))
end

function ProcessHorizontalMovement(deltaTime)
    PrintToConsole("[ProcessHorizontal] Called with input: " .. Config.HorizontalInput)

    if not MyActor.GetRightDirection then
        PrintToConsole("[ProcessHorizontal] ERROR: GetRightDirection not found!")
        return
    end

    if not MyActor.AddMovementInput then
        PrintToConsole("[ProcessHorizontal] ERROR: AddMovementInput not found!")
        return
    end

    -- 우측 방향 가져오기 (C++에서 중력 방향 고려해서 계산)
    local rightDir = MyActor:GetRightDirection()
    PrintToConsole(string.format("[ProcessHorizontal] RightDir: (%.2f, %.2f, %.2f)",
        rightDir.X, rightDir.Y, rightDir.Z))

    if not rightDir then
        PrintToConsole("[ProcessHorizontal] ERROR: rightDir is nil!")
        return
    end

    -- FVector 생성하여 전달 (입력 크기는 HorizontalInput * StrafeSpeed 사용)
    local success, result = pcall(function()
        local scaledDir = FVector(
            rightDir.X * Config.HorizontalInput * Config.StrafeSpeed,
            rightDir.Y * Config.HorizontalInput * Config.StrafeSpeed,
            rightDir.Z * Config.HorizontalInput * Config.StrafeSpeed
        )
        return scaledDir
    end)

    if not success then
        PrintToConsole("[ProcessHorizontal] ERROR creating FVector: " .. tostring(result))
        return
    end

    local scaledDir = result
    PrintToConsole(string.format("[ProcessHorizontal] ScaledDir: (%.2f, %.2f, %.2f)",
        scaledDir.X, scaledDir.Y, scaledDir.Z))

    -- AddMovementInput 호출: (FVector, float)
    local success2, err2 = pcall(function()
        MyActor:AddMovementInput(scaledDir, 1.0)
    end)

    if success2 then
        PrintToConsole("[ProcessHorizontal] AddMovementInput called!")
    else
        PrintToConsole("[ProcessHorizontal] ERROR calling AddMovementInput: " .. tostring(err2))
    end
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
    if Config.bDebugLog then
        PrintToConsole("[RunnerCharacter] ⚡ Overlapped: " .. OtherActor:GetName():ToString())
    end

    -- TODO: 벽면 감지 및 중력 방향 전환 로직
    -- 예: OtherActor의 태그를 확인해서 "WallTrigger"면 중력 방향 변경
end

-- ════════════════════════════════════════════════════════════════════════════
-- 스크립트 로드 완료
-- ════════════════════════════════════════════════════════════════════════════

PrintToConsole("[RunnerCharacter] ✓ Lua script loaded successfully!")
