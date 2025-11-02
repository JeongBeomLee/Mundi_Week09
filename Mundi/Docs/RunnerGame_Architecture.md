# 🎮 런2 게임 완전 설계 문서

## 목차

1. [개요](#개요)
2. [아키텍처 전략](#아키텍처-전략)
3. [시스템 구성](#시스템-구성)
4. [GameMode & GameState](#gamemode--gamestate)
5. [코루틴 시스템](#코루틴-시스템)
6. [Lua 중심 게임 로직](#lua-중심-게임-로직)
7. [구현 순서](#구현-순서)
8. [파일 구조](#파일-구조)

---

## 개요

### 핵심 전략: "C++는 엔진, Lua는 게임"

- **C++**: 최소한의 베이스 클래스만 (Character, GameMode, GameState, Obstacle)
- **Lua**: 모든 게임 로직, AI, 스폰, 난이도, UI, 이벤트
- **코루틴**: 복잡한 시퀀스, 패턴, 애니메이션을 순차적으로 처리

### 활용 가능한 시스템

- ✅ **Character + CharacterMovementComponent** (이동/점프)
- ✅ **PlayerController + InputComponent** (입력 처리)
- ✅ **Delegate 시스템** (이벤트 처리)
- ✅ **Lua 스크립팅** (게임 로직)
- ✅ **충돌 시스템** (CollisionComponent, BVH)
- ✅ **GameMode + GameState** (게임 규칙 및 상태)
- ✅ **코루틴 시스템** (시퀀스 제어)

---

## 아키텍처 전략

### C++ vs Lua 비율

- **C++ (30%)**: Character, Movement, Collision, Rendering, Input 처리
- **Lua (70%)**: 게임 로직, AI, 스폰, 난이도, UI, 이벤트, 데이터

### 장점

1. ✅ **빠른 반복**: Lua 수정 후 바로 Hot Reload
2. ✅ **모드 지원**: 유저가 Lua 파일 수정해서 커스텀 가능
3. ✅ **데이터 주도**: 모든 게임 설정이 Lua 테이블
4. ✅ **디버깅**: PrintToConsole로 즉시 확인
5. ✅ **프로토타이핑**: C++ 빌드 없이 기능 테스트

---

## 시스템 구성

### 전체 아키텍처

```
┌─────────────────────────────────────────────────────────┐
│                      UWorld                             │
├─────────────────────────────────────────────────────────┤
│  GameMode: ARunnerGameMode (C++)                        │
│    └─> Lua Script: runner_game_mode.lua                │
│                                                          │
│  GameState: ARunnerGameState (C++)                      │
│    ├─> Score, Coins, Distance                           │
│    ├─> Difficulty, Wave                                 │
│    ├─> Lives, Combo                                     │
│    └─> Delegates → Lua callbacks                        │
│                                                          │
│  CoroutineManager                                        │
│    └─> Lua coroutines (시퀀스, 패턴, 애니메이션)         │
│                                                          │
│  Actors:                                                 │
│    ├─> ARunnerCharacter + runner_character.lua          │
│    ├─> AObstacle + obstacle_*.lua                       │
│    └─> ACoin + coin.lua                                 │
└─────────────────────────────────────────────────────────┘
```

---

## GameMode & GameState

### Phase 1: GameStateBase (C++)

게임의 복제 가능한 상태 정보를 저장합니다.

```cpp
// GameStateBase.h
#pragma once

#include "Info.h"
#include "Delegate.h"

class AGameStateBase : public AInfo
{
public:
    DECLARE_CLASS(AGameStateBase, AInfo)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(AGameStateBase)

    AGameStateBase();
    virtual ~AGameStateBase() override;

    // 게임 상태
    bool HasMatchStarted() const { return bMatchStarted; }
    bool IsMatchInProgress() const { return bMatchInProgress; }
    bool HasMatchEnded() const { return bMatchEnded; }
    float GetElapsedTime() const { return ElapsedTime; }

    void SetMatchStarted(bool bStarted);
    void SetMatchInProgress(bool bInProgress);
    void SetMatchEnded(bool bEnded);

    // 델리게이트
    DECLARE_MULTICAST_DELEGATE(FOnMatchStateChanged)
    FOnMatchStateChanged OnMatchStateChanged;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    bool bMatchStarted = false;
    bool bMatchInProgress = false;
    bool bMatchEnded = false;
    float ElapsedTime = 0.0f;
};
```

### Phase 2: GameModeBase (C++)

게임의 규칙과 로직을 정의합니다.

```cpp
// GameModeBase.h
#pragma once

#include "Info.h"
#include "Delegate.h"

class AGameStateBase;

class AGameModeBase : public AInfo
{
public:
    DECLARE_CLASS(AGameModeBase, AInfo)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(AGameModeBase)

    AGameModeBase();
    virtual ~AGameModeBase() override;

    // 게임 흐름 제어
    virtual void StartMatch();
    virtual void PauseMatch();
    virtual void ResumeMatch();
    virtual void EndMatch();
    virtual void RestartMatch();

    // GameState 접근
    AGameStateBase* GetGameState() const { return GameState; }

    template<typename T>
    T* GetGameState() const { return Cast<T>(GameState); }

    // 델리게이트
    DECLARE_MULTICAST_DELEGATE(FOnMatchStarted)
    FOnMatchStarted OnMatchStarted;

    DECLARE_MULTICAST_DELEGATE(FOnMatchPaused)
    FOnMatchPaused OnMatchPaused;

    DECLARE_MULTICAST_DELEGATE(FOnMatchResumed)
    FOnMatchResumed OnMatchResumed;

    DECLARE_MULTICAST_DELEGATE(FOnMatchEnded)
    FOnMatchEnded OnMatchEnded;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void InitGameState();

    TSubclassOf<AGameStateBase> GameStateClass;
    AGameStateBase* GameState = nullptr;
};
```

### Phase 3: RunnerGameState (C++)

런2 게임 전용 상태 정보

```cpp
// RunnerGameState.h
#pragma once

#include "GameStateBase.h"

class ARunnerGameState : public AGameStateBase
{
public:
    DECLARE_CLASS(ARunnerGameState, AGameStateBase)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(ARunnerGameState)

    ARunnerGameState();
    virtual ~ARunnerGameState() override;

    // 점수 시스템
    int32 GetScore() const { return Score; }
    void SetScore(int32 NewScore);
    void AddScore(int32 Delta);

    int32 GetCoins() const { return Coins; }
    void SetCoins(int32 NewCoins);
    void AddCoins(int32 Delta);

    float GetDistance() const { return Distance; }
    void SetDistance(float NewDistance) { Distance = NewDistance; }

    int32 GetHighScore() const { return HighScore; }
    void SetHighScore(int32 NewHighScore);

    // 난이도
    float GetDifficulty() const { return Difficulty; }
    void SetDifficulty(float NewDifficulty) { Difficulty = NewDifficulty; }

    int32 GetCurrentWave() const { return CurrentWave; }
    void SetCurrentWave(int32 Wave) { CurrentWave = Wave; }

    // 플레이어 상태
    bool IsPlayerAlive() const { return bPlayerAlive; }
    void SetPlayerAlive(bool bAlive);

    int32 GetLives() const { return Lives; }
    void SetLives(int32 NewLives) { Lives = NewLives; }

    // 콤보 시스템
    int32 GetCombo() const { return Combo; }
    void SetCombo(int32 NewCombo);
    void IncrementCombo();
    void ResetCombo();
    float GetComboMultiplier() const;

    // 델리게이트
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32)
    FOnScoreChanged OnScoreChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnCoinsChanged, int32)
    FOnCoinsChanged OnCoinsChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnComboChanged, int32)
    FOnComboChanged OnComboChanged;

    DECLARE_MULTICAST_DELEGATE(FOnPlayerDied)
    FOnPlayerDied OnPlayerDied;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    int32 Score = 0;
    int32 Coins = 0;
    float Distance = 0.0f;
    int32 HighScore = 0;

    float Difficulty = 1.0f;
    int32 CurrentWave = 0;

    bool bPlayerAlive = true;
    int32 Lives = 3;

    int32 Combo = 0;
    float ComboTimer = 0.0f;
    float ComboTimeout = 3.0f;
};
```

### Phase 4: RunnerGameMode (C++)

런2 게임 전용 게임 모드

```cpp
// RunnerGameMode.h
#pragma once

#include "GameModeBase.h"

class ARunnerGameState;
class ARunnerCharacter;

class ARunnerGameMode : public AGameModeBase
{
public:
    DECLARE_CLASS(ARunnerGameMode, AGameModeBase)
    GENERATED_REFLECTION_BODY()
    DECLARE_DUPLICATE(ARunnerGameMode)

    ARunnerGameMode();
    virtual ~ARunnerGameMode() override;

    // 게임 이벤트
    void OnPlayerDeath(ARunnerCharacter* Player);
    void OnCoinCollected(int32 CoinValue);
    void OnObstacleAvoided();
    void OnPlayerJump();

    // 난이도 조절
    void UpdateDifficulty(float DeltaTime);
    float GetCurrentSpawnInterval() const;

    // RunnerGameState 접근
    ARunnerGameState* GetRunnerGameState() const;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    virtual void OnMatchStarted_Internal() override;
    virtual void OnMatchEnded_Internal() override;

    ARunnerCharacter* FindPlayerCharacter();
    void BindPlayerEvents();

    float BaseDifficulty = 1.0f;
    float DifficultyIncreaseRate = 0.1f;

    int32 JumpScore = 10;
    int32 CoinScore = 50;
    int32 AvoidScore = 20;

    ARunnerCharacter* PlayerCharacter = nullptr;
};
```

---

## 코루틴 시스템

### Phase 1: CoroutineManager (C++)

Lua 코루틴을 관리하는 시스템

```cpp
// CoroutineManager.h
#pragma once

#include "Object.h"
#include <sol/sol.hpp>

struct FCoroutineHandle
{
    sol::coroutine Coroutine;
    FString Name;
    bool bIsPaused = false;
    float WaitTime = 0.0f;
    int32 WaitFrames = 0;

    FCoroutineHandle() = default;
    FCoroutineHandle(sol::coroutine InCoroutine, const FString& InName)
        : Coroutine(InCoroutine), Name(InName) {}
};

class UCoroutineManager : public UObject
{
    DECLARE_CLASS(UCoroutineManager, UObject)

public:
    UCoroutineManager();
    ~UCoroutineManager() override;

    static UCoroutineManager& GetInstance();

    // 코루틴 시작
    int32 StartCoroutine(sol::coroutine Co, const FString& Name = "");

    // 코루틴 중지/재개
    void StopCoroutine(int32 Handle);
    void PauseCoroutine(int32 Handle);
    void ResumeCoroutine(int32 Handle);
    void StopAllCoroutines();

    // 매 프레임 업데이트
    void UpdateCoroutines(float DeltaTime);

    // 코루틴 상태 확인
    bool IsRunning(int32 Handle) const;
    int32 GetActiveCoroutineCount() const { return ActiveCoroutines.size(); }

private:
    TMap<int32, FCoroutineHandle> ActiveCoroutines;
    int32 NextHandle = 0;
};
```

### Phase 2: Lua 바인딩

```cpp
// UScriptManager::RegisterGlobalFuncToLua()에 추가

void UScriptManager::RegisterGlobalFuncToLua()
{
    // 코루틴 매니저 바인딩
    Lua.new_usertype<UCoroutineManager>("CoroutineManager",
        sol::no_constructor,
        "StartCoroutine", &UCoroutineManager::StartCoroutine,
        "StopCoroutine", &UCoroutineManager::StopCoroutine,
        "PauseCoroutine", &UCoroutineManager::PauseCoroutine,
        "ResumeCoroutine", &UCoroutineManager::ResumeCoroutine,
        "StopAllCoroutines", &UCoroutineManager::StopAllCoroutines,
        "IsRunning", &UCoroutineManager::IsRunning
    );

    // 전역 함수로 편하게 사용
    Lua["StartCoroutine"] = [](sol::function Func, sol::this_state s) -> int32 {
        sol::state_view lua(s);
        sol::coroutine Co = sol::coroutine::create(lua, Func);
        return UCoroutineManager::GetInstance().StartCoroutine(Co);
    };

    Lua["StopCoroutine"] = [](int32 Handle) {
        UCoroutineManager::GetInstance().StopCoroutine(Handle);
    };

    // 대기 헬퍼 함수
    Lua["WaitForSeconds"] = [](float Seconds) -> sol::table {
        sol::state_view lua;
        sol::table Result = lua.create_table();
        Result["type"] = "wait";
        Result["time"] = Seconds;
        return Result;
    };

    Lua["WaitForFrames"] = [](int32 Frames) -> sol::table {
        sol::state_view lua;
        sol::table Result = lua.create_table();
        Result["type"] = "waitframes";
        Result["frames"] = Frames;
        return Result;
    };
}
```

### Phase 3: World Tick에 추가

```cpp
// World.cpp 또는 GameMode.cpp의 Tick에서
void UWorld::Tick(float DeltaTime)
{
    // 기존 코드...

    // 코루틴 업데이트
    UCoroutineManager::GetInstance().UpdateCoroutines(DeltaTime);
}
```

---

## Lua 중심 게임 로직

### 파일 구조

```
Scripts/
├── player/
│   ├── runner_character.lua      # 플레이어 로직
│   ├── player_input.lua           # 입력 처리
│   └── player_animation.lua       # 애니메이션 상태
├── obstacles/
│   ├── obstacle_wall.lua          # 벽 장애물
│   ├── obstacle_barrier.lua       # 낮은 장애물
│   ├── obstacle_coin.lua          # 코인
│   ├── obstacle_moving.lua        # 움직이는 장애물
│   └── obstacle_boss.lua          # 보스 장애물
├── game/
│   ├── runner_game_mode.lua       # 게임 모드 (메인 로직)
│   ├── level_spawner.lua          # 레벨 생성
│   ├── difficulty_manager.lua     # 난이도 조절
│   ├── wave_manager.lua           # 웨이브 시스템
│   └── score_manager.lua          # 점수/코인 관리
├── ui/
│   ├── hud.lua                    # 게임 HUD
│   ├── game_over.lua              # 게임오버 화면
│   └── pause_menu.lua             # 일시정지 메뉴
├── camera/
│   └── camera_effects.lua         # 카메라 효과
├── powerups/
│   ├── magnet_powerup.lua         # 자석 파워업
│   └── shield_powerup.lua         # 쉴드 파워업
└── utils/
    ├── math_utils.lua             # 수학 유틸리티
    └── tween.lua                  # 보간 함수
```

### 메인 게임 모드 (Lua)

```lua
-- game/runner_game_mode.lua

local gameMode = nil
local gameState = nil

function BeginPlay()
    -- C++ GameMode/GameState 참조 획득
    gameMode = GetGameMode()
    gameState = GetGameState()

    if not gameMode or not gameState then
        PrintToConsole("[ERROR] GameMode or GameState not found!")
        return
    end

    PrintToConsole("[Runner Game Mode] Initializing...")

    -- 전역으로 등록
    _G.GameMode = gameMode
    _G.GameState = gameState

    -- 이벤트 구독
    SubscribeToEvents()

    -- 게임 시작 시퀀스
    StartCoroutine(GameStartSequence)
end

-- 게임 시작 시퀀스 (코루틴)
function GameStartSequence()
    PrintToConsole("=== RUNNER GAME ===")

    -- 카운트다운
    for i = 3, 1, -1 do
        ShowMessage("GET READY... " .. i)
        coroutine.yield(1.0)
    end

    ShowMessage("GO!")
    coroutine.yield(0.5)
    HideMessage()

    -- 매치 시작
    gameMode:StartMatch()

    -- 레벨 스포너 시작
    StartCoroutine(require("game/level_spawner").StartSpawning)

    -- 난이도 매니저 시작
    StartCoroutine(require("game/difficulty_manager").ManageDifficulty)
end

-- 이벤트 구독
function SubscribeToEvents()
    -- GameState 이벤트 (C++ 델리게이트)
    -- gameState.OnScoreChanged:Add(OnScoreChanged)
    -- gameState.OnCoinsChanged:Add(OnCoinsChanged)
    -- gameState.OnPlayerDied:Add(OnPlayerDied)
end

-- 점수 변경 시
function OnScoreChanged(newScore)
    PrintToConsole("[Score] " .. newScore)

    -- UI 업데이트
    UpdateHUD()

    -- 특정 점수마다 이벤트
    if newScore >= 1000 and newScore < 1010 then
        ShowMessage("1000 POINTS!")
        StartCoroutine(CelebrationEffect)
    end
end

-- 코인 변경 시
function OnCoinsChanged(newCoins)
    PrintToConsole("[Coins] " .. newCoins)

    -- 10개마다 보너스
    if newCoins % 10 == 0 and newCoins > 0 then
        gameState:AddScore(500)
        ShowMessage("10 COINS BONUS! +500")
    end
end

-- 플레이어 사망 시
function OnPlayerDied()
    PrintToConsole("[Game Mode] Player Died!")

    -- 사망 연출
    StartCoroutine(DeathSequence)
end

-- 사망 연출 (코루틴)
function DeathSequence()
    -- 슬로우 모션
    SetTimeScale(0.3)
    coroutine.yield(1.0)
    SetTimeScale(1.0)

    -- 카메라 쉐이크
    StartCoroutine(CameraShake, 0.5, 30.0)

    coroutine.yield(1.0)

    -- 남은 생명 확인
    local lives = gameState:GetLives()

    if lives > 0 then
        -- 리스폰
        ShowMessage("Lives: " .. lives)
        coroutine.yield(2.0)

        gameState:SetPlayerAlive(true)
        RespawnPlayer()
    else
        -- 게임 오버
        StartCoroutine(GameOverSequence)
    end
end

-- 게임 오버 시퀀스 (코루틴)
function GameOverSequence()
    -- 매치 종료
    gameMode:EndMatch()

    coroutine.yield(1.0)

    -- 게임 오버 화면 표시
    local finalScore = gameState:GetScore()
    local finalCoins = gameState:GetCoins()
    local finalDistance = gameState:GetDistance()
    local highScore = gameState:GetHighScore()

    PrintToConsole("========================================")
    PrintToConsole("           GAME OVER")
    PrintToConsole("========================================")
    PrintToConsole("  Final Score: " .. finalScore)
    PrintToConsole("  Coins: " .. finalCoins)
    PrintToConsole("  Distance: " .. string.format("%.1fm", finalDistance))
    PrintToConsole("  High Score: " .. highScore)
    PrintToConsole("========================================")

    if finalScore >= highScore then
        PrintToConsole("  ★★★ NEW HIGH SCORE! ★★★")
        StartCoroutine(HighScoreCelebration)
    end

    coroutine.yield(2.0)

    PrintToConsole("  Press R to Restart")

    -- 재시작 대기
    WaitForRestartInput()
end

-- 축하 효과
function CelebrationEffect()
    for i = 1, 5 do
        ShowMessage("★ NICE! ★")
        coroutine.yield(0.2)
        HideMessage()
        coroutine.yield(0.2)
    end
end

function Tick(dt)
    -- 게임 진행 중이면 거리 업데이트
    if gameState and gameState:IsMatchInProgress() then
        local currentDistance = gameState:GetDistance()
        gameState:SetDistance(currentDistance + dt * 8.0)  -- 8m/s
    end
end

function EndPlay()
    PrintToConsole("[Runner Game Mode] Shutdown")
end
```

### 플레이어 (Lua)

```lua
-- player/runner_character.lua

local player = {
    -- 레인 시스템
    currentLane = 0,        -- -1, 0, 1
    laneWidth = 300.0,
    laneSpeed = 10.0,

    -- 자동 런
    runSpeed = 800.0,

    -- 슬라이드
    isSliding = false,
    slideTimer = 0.0,
    slideDuration = 1.0,
}

function BeginPlay()
    PrintToConsole("[Runner] Player Started!")

    if MyActor then
        MyActor:SetAutoRun(true)
        MyActor:SetRunSpeed(player.runSpeed)
    end
end

-- 점프 (C++ InputComponent에서 호출)
function OnJumpPressed()
    local gameState = GetGameState()

    if gameState and gameState:IsPlayerAlive() and MyActor:CanJump() then
        MyActor:Jump()

        -- GameMode에 알림
        local gameMode = GetGameMode()
        if gameMode then
            gameMode:OnPlayerJump()
        end

        -- 콤보 증가
        gameState:IncrementCombo()
    end
end

function OnJumpReleased()
    MyActor:StopJumping()
end

-- 슬라이드 (C++ InputComponent에서 호출)
function OnSlidePressed()
    local gameState = GetGameState()

    if gameState and gameState:IsPlayerAlive() and not player.isSliding then
        StartSlide()
    end
end

function StartSlide()
    player.isSliding = true
    player.slideTimer = 0.0
    MyActor:Crouch()

    local gameState = GetGameState()
    if gameState then
        gameState:AddScore(5)
        gameState:IncrementCombo()
    end
end

function StopSlide()
    player.isSliding = false
    MyActor:UnCrouch()
end

-- 레인 이동 (C++ InputComponent에서 호출)
function OnMoveLeft()
    local gameState = GetGameState()

    if gameState and gameState:IsPlayerAlive() and player.currentLane > -1 then
        player.currentLane = player.currentLane - 1
    end
end

function OnMoveRight()
    local gameState = GetGameState()

    if gameState and gameState:IsPlayerAlive() and player.currentLane < 1 then
        player.currentLane = player.currentLane + 1
    end
end

-- 매 프레임 업데이트
function Tick(dt)
    local gameState = GetGameState()

    if not gameState or not gameState:IsMatchInProgress() then
        return
    end

    -- 자동 전진
    local forward = FVector(0, 1, 0)
    MyActor:AddMovementInput(forward, 1.0)

    -- 레인 보간 이동
    UpdateLanePosition(dt)

    -- 슬라이드 타이머
    if player.isSliding then
        player.slideTimer = player.slideTimer + dt
        if player.slideTimer >= player.slideDuration then
            StopSlide()
        end
    end
end

-- 레인 보간 이동
function UpdateLanePosition(dt)
    local targetX = player.currentLane * player.laneWidth
    local currentPos = MyActor:GetLocation()
    local newX = Lerp(currentPos.X, targetX, player.laneSpeed * dt)

    currentPos.X = newX
    MyActor:SetLocation(currentPos)
end

-- 충돌 처리
function OnOverlap(OtherActor)
    local gameState = GetGameState()

    if not gameState or not gameState:IsPlayerAlive() then
        return
    end

    local actorName = OtherActor:GetName():ToString()

    if string.find(actorName, "Obstacle") then
        OnHitObstacle(OtherActor)
    elseif string.find(actorName, "Coin") then
        OnCollectCoin(OtherActor)
    end
end

-- 장애물 충돌
function OnHitObstacle(obstacle)
    local gameState = GetGameState()
    local gameMode = GetGameMode()

    if gameState and gameMode then
        gameState:SetPlayerAlive(false)
        gameState:ResetCombo()

        gameMode:OnPlayerDeath(MyActor)

        MyActor:SetAutoRun(false)
    end
end

-- 코인 수집
function OnCollectCoin(coin)
    local gameMode = GetGameMode()

    if gameMode then
        gameMode:OnCoinCollected(1)
    end

    coin:Destroy()
end

-- 유틸리티
function Lerp(a, b, t)
    return a + (b - a) * math.min(t, 1.0)
end

function EndPlay()
    PrintToConsole("[Runner] Player Destroyed")
end

-- 외부에서 접근 가능하도록 export
_G.Player = player
```

### 장애물 - 벽 (Lua)

```lua
-- obstacles/obstacle_wall.lua

local obstacle = {
    moveSpeed = -800.0,
    destroyDistance = -1000.0,
    lane = 0,
}

function BeginPlay()
    -- 랜덤 레인에 배치
    obstacle.lane = math.random(-1, 1)

    local pos = MyActor:GetLocation()
    pos.X = obstacle.lane * 300.0
    MyActor:SetLocation(pos)

    PrintToConsole("[Obstacle Wall] Spawned at lane: " .. obstacle.lane)
end

function Tick(dt)
    -- 플레이어 방향으로 이동
    local pos = MyActor:GetLocation()
    pos.Y = pos.Y + obstacle.moveSpeed * dt
    MyActor:SetLocation(pos)

    -- 화면 밖으로 나가면 제거
    if pos.Y < obstacle.destroyDistance then
        MyActor:Destroy()
    end
end

function OnOverlap(OtherActor)
    local actorName = OtherActor:GetName():ToString()

    if string.find(actorName, "Runner") or string.find(actorName, "Character") then
        PrintToConsole("[Obstacle Wall] Hit Player!")
    end
end

function EndPlay()
end
```

### 장애물 - 코인 (Lua)

```lua
-- obstacles/obstacle_coin.lua

local coin = {
    moveSpeed = -800.0,
    destroyDistance = -1000.0,
    lane = 0,
    rotationSpeed = 180.0,
}

function BeginPlay()
    coin.lane = math.random(-1, 1)

    local pos = MyActor:GetLocation()
    pos.X = coin.lane * 300.0
    pos.Z = 100.0  -- 공중에 떠있음
    MyActor:SetLocation(pos)

    PrintToConsole("[Coin] Spawned")
end

function Tick(dt)
    -- 이동
    local pos = MyActor:GetLocation()
    pos.Y = pos.Y + coin.moveSpeed * dt
    MyActor:SetLocation(pos)

    -- 회전 (보기 좋게)
    local rot = MyActor:GetRotation()
    local euler = FVector(0, 0, coin.rotationSpeed * dt)
    local deltaQuat = FQuat.MakeFromEuler(euler.X, euler.Y, euler.Z)
    MyActor:AddWorldRotation(deltaQuat)

    -- 화면 밖이면 제거
    if pos.Y < coin.destroyDistance then
        MyActor:Destroy()
    end
end

function OnOverlap(OtherActor)
    -- 플레이어가 수집 (플레이어 스크립트에서 Destroy 호출)
end

function EndPlay()
end
```

### 보스 장애물 (Lua + 코루틴)

```lua
-- obstacles/obstacle_boss.lua

local boss = {
    health = 3,
    phase = 1,
    speed = -600,
}

function BeginPlay()
    PrintToConsole("[Boss] Spawned!")
    StartCoroutine(BossBehavior)
end

-- 보스 AI (코루틴)
function BossBehavior()
    -- Phase 1: 느리게 접근
    boss.phase = 1
    PrintToConsole("[Boss] Phase 1: Slow approach")
    boss.speed = -400
    coroutine.yield(3.0)

    -- Phase 2: 좌우 이동하며 접근
    boss.phase = 2
    PrintToConsole("[Boss] Phase 2: Weaving")
    boss.speed = -600
    local weaveHandle = StartCoroutine(WeavePattern)
    coroutine.yield(4.0)
    StopCoroutine(weaveHandle)

    -- Phase 3: 빠르게 돌진!
    boss.phase = 3
    PrintToConsole("[Boss] Phase 3: CHARGE!")
    boss.speed = -1500

    -- 경고 효과 (깜빡임)
    StartCoroutine(BlinkWarning)
    coroutine.yield(2.0)
end

function WeavePattern()
    local amplitude = 200
    local frequency = 2.0
    local time = 0

    while true do
        time = time + 0.1
        local offset = math.sin(time * frequency) * amplitude

        local pos = MyActor:GetLocation()
        pos.X = offset
        MyActor:SetLocation(pos)

        coroutine.yield(0.1)
    end
end

function BlinkWarning()
    for i = 1, 6 do
        -- 빨간색
        SetColor(FColor(255, 0, 0))
        coroutine.yield(0.2)

        -- 원래 색
        SetColor(FColor(255, 255, 255))
        coroutine.yield(0.2)
    end
end

function Tick(dt)
    local pos = MyActor:GetLocation()
    pos.Y = pos.Y + boss.speed * dt
    MyActor:SetLocation(pos)
end
```

### 웨이브 스포너 (Lua + 코루틴)

```lua
-- game/wave_spawner.lua

local waves = {
    -- Wave 1: 간단한 패턴
    {
        name = "Wave 1: Warm up",
        duration = 10.0,
        pattern = function()
            SpawnWall()
            coroutine.yield(2.0)

            SpawnBarrier()
            coroutine.yield(2.0)

            SpawnCoin()
            coroutine.yield(2.0)
        end
    },

    -- Wave 2: 연속 장애물
    {
        name = "Wave 2: Combo",
        duration = 15.0,
        pattern = function()
            for i = 1, 3 do
                SpawnWall()
                coroutine.yield(0.5)
            end

            coroutine.yield(2.0)

            SpawnBarrier()
            coroutine.yield(0.3)
            SpawnCoin()
        end
    },

    -- Wave 3: 보스
    {
        name = "Wave 3: BOSS!",
        duration = 20.0,
        pattern = function()
            PrintToConsole(">>> BOSS INCOMING! <<<")

            for i = 3, 1, -1 do
                ShowMessage("BOSS in " .. i .. "...")
                coroutine.yield(1.0)
            end

            SpawnBoss()

            local coinHandle = StartCoroutine(SpawnCoinsAroundBoss)

            coroutine.yield(15.0)
            StopCoroutine(coinHandle)
        end
    }
}

function RunWaveSystem()
    for i, wave in ipairs(waves) do
        PrintToConsole("=== " .. wave.name .. " ===")

        local patternHandle = StartCoroutine(wave.pattern)

        coroutine.yield(wave.duration)

        StopCoroutine(patternHandle)

        PrintToConsole("--- Break Time ---")
        coroutine.yield(3.0)
    end

    PrintToConsole("=== All Waves Cleared! Infinite Mode... ===")

    StartCoroutine(InfiniteMode)
end

function SpawnCoinsAroundBoss()
    while true do
        SpawnCoinAt(math.random(-1, 1) * 300, 1500)
        coroutine.yield(0.5)
    end
end
```

### 카메라 효과 (Lua + 코루틴)

```lua
-- camera/camera_effects.lua

-- 카메라 쉐이크
function CameraShake(duration, intensity)
    local originalPos = Camera:GetLocation()
    local elapsed = 0

    while elapsed < duration do
        local offsetX = (math.random() - 0.5) * intensity
        local offsetY = (math.random() - 0.5) * intensity
        local offsetZ = (math.random() - 0.5) * intensity

        local shakePos = originalPos + FVector(offsetX, offsetY, offsetZ)
        Camera:SetLocation(shakePos)

        elapsed = elapsed + 0.05
        coroutine.yield(0.05)
    end

    Camera:SetLocation(originalPos)
end

-- 슬로우 모션
function SlowMotion(duration, timeScale)
    SetTimeScale(timeScale)
    coroutine.yield(duration)
    SetTimeScale(1.0)
end

-- 플레이어가 죽었을 때
function OnPlayerDeath()
    StartCoroutine(function()
        SlowMotion(1.0, 0.3)

        StartCoroutine(function()
            CameraShake(0.5, 20.0)
        end)
    end)
end
```

### 파워업 - 자석 (Lua + 코루틴)

```lua
-- powerups/magnet_powerup.lua

function OnPlayerCollect()
    PrintToConsole("[Powerup] Magnet activated!")
    StartCoroutine(MagnetEffect)
end

function MagnetEffect()
    local duration = 10.0
    local elapsed = 0
    local magnetRange = 500.0

    PrintToConsole("[Magnet] Active for " .. duration .. " seconds")

    while elapsed < duration do
        local playerPos = GetPlayerPosition()
        local coins = FindAllCoinsInRange(playerPos, magnetRange)

        for _, coin in ipairs(coins) do
            StartCoroutine(function()
                MoveTowardsPlayer(coin, 5.0)
            end)
        end

        elapsed = elapsed + 0.5
        coroutine.yield(0.5)
    end

    PrintToConsole("[Magnet] Effect ended")
end

function MoveTowardsPlayer(coin, duration)
    local elapsed = 0
    local startPos = coin:GetLocation()

    while elapsed < duration do
        local playerPos = GetPlayerPosition()
        local t = elapsed / duration

        local currentPos = LerpVector(startPos, playerPos, t)
        coin:SetLocation(currentPos)

        elapsed = elapsed + 0.05
        coroutine.yield(0.05)
    end

    CollectCoin(coin)
end
```

---

## 구현 순서

### Week 1: 기본 시스템 구축

1. **GameMode & GameState C++ 기반 클래스**
   - GameStateBase.h/cpp
   - GameModeBase.h/cpp
   - World에 GameMode 추가
   - 프로젝트 파일 업데이트

2. **코루틴 시스템**
   - CoroutineManager.h/cpp
   - Lua 바인딩
   - World Tick에 UpdateCoroutines 추가

3. **RunnerGameState & RunnerGameMode**
   - RunnerGameState.h/cpp (점수, 난이도, 콤보)
   - RunnerGameMode.h/cpp (게임 규칙)

### Week 2: 플레이어 시스템

1. **RunnerCharacter (C++)**
   - ACharacter 상속
   - 레인 시스템 기본 구조
   - 자동 전진 로직
   - 델리게이트 추가

2. **플레이어 Lua 스크립트**
   - runner_character.lua
   - 입력 처리
   - GameState 연동
   - 충돌 처리

### Week 3: 장애물 & 스폰 시스템

1. **장애물 베이스 클래스 (C++)**
   - AObstacle 클래스
   - 충돌 컴포넌트

2. **장애물 Lua 스크립트**
   - obstacle_wall.lua
   - obstacle_barrier.lua
   - obstacle_coin.lua
   - obstacle_boss.lua (코루틴 패턴)

3. **레벨 스포너 (Lua)**
   - level_spawner.lua
   - wave_spawner.lua (코루틴 웨이브)

### Week 4: 게임 로직 & UI

1. **게임 모드 Lua 스크립트**
   - runner_game_mode.lua
   - 게임 시작/종료 시퀀스 (코루틴)
   - 이벤트 처리

2. **UI (Lua)**
   - hud.lua
   - game_over.lua
   - 스코어 애니메이션 (코루틴)

3. **파워업 시스템 (Lua + 코루틴)**
   - magnet_powerup.lua
   - shield_powerup.lua

### Week 5: 폴리싱

1. **카메라 시스템**
   - 3인칭 추적 카메라
   - 카메라 효과 (쉐이크, 슬로우모션)

2. **사운드 & 이펙트**
   - 파티클 시스템 연동
   - 사운드 재생

3. **밸런싱**
   - 난이도 조정
   - 점수 밸런싱
   - 플레이테스트

---

## 코루틴 활용 사례

### 1. 시퀀스 연출

```lua
function GameStartSequence()
    for i = 3, 1, -1 do
        ShowMessage("GET READY... " .. i)
        coroutine.yield(1.0)
    end
    ShowMessage("GO!")
end
```

### 2. 복잡한 AI 패턴

```lua
function BossBehavior()
    boss.speed = -400
    coroutine.yield(3.0)

    local weaveHandle = StartCoroutine(WeavePattern)
    coroutine.yield(4.0)
    StopCoroutine(weaveHandle)

    boss.speed = -1500
end
```

### 3. 애니메이션

```lua
function CountUpScore()
    while displayScore < targetScore do
        displayScore = displayScore + increment
        UpdateScoreUI(displayScore)
        coroutine.yield(0.05)
    end
end
```

### 4. 조건부 대기

```lua
function WaitForPlayerJump()
    _G.TutorialWaitingForJump = true

    while _G.TutorialWaitingForJump do
        coroutine.yield(0.1)
    end
end
```

### 5. 이펙트

```lua
function CameraShake(duration, intensity)
    local originalPos = Camera:GetLocation()
    local elapsed = 0

    while elapsed < duration do
        local offset = RandomVector() * intensity
        Camera:SetLocation(originalPos + offset)

        elapsed = elapsed + 0.05
        coroutine.yield(0.05)
    end

    Camera:SetLocation(originalPos)
end
```

---

## 핵심 포인트

### GameState = 데이터

- 모든 게임 상태는 GameState에 저장
- Lua에서 쉽게 접근 가능
- 델리게이트로 변경 알림

### GameMode = 로직

- 게임 규칙과 이벤트 처리
- Lua에서 대부분 구현 가능
- C++는 최소한의 프레임워크만

### 코루틴 = 시퀀스

- 복잡한 게임 흐름을 순차적으로
- 시작/죽음/게임오버 시퀀스
- 애니메이션, 패턴, 웨이브

### Lua = 게임

- 모든 게임 로직은 Lua
- Hot Reload로 빠른 반복
- 데이터 주도 설계

---

## 참고 자료

### C++ 클래스 다이어그램

```
AActor
  └─> AInfo
        ├─> AGameStateBase
        │     └─> ARunnerGameState (점수, 난이도, 콤보)
        │
        └─> AGameModeBase
              └─> ARunnerGameMode (게임 규칙, 이벤트)

APawn
  └─> ACharacter
        └─> ARunnerCharacter (레인, 자동런, 슬라이드)

AActor
  └─> AObstacle (장애물 베이스)
        ├─> + obstacle_wall.lua
        ├─> + obstacle_barrier.lua
        ├─> + obstacle_coin.lua
        └─> + obstacle_boss.lua
```

### Lua 모듈 의존성

```
runner_game_mode.lua
  ├─> level_spawner.lua
  ├─> difficulty_manager.lua
  ├─> wave_spawner.lua
  └─> hud.lua

runner_character.lua
  ├─> GameState (C++)
  └─> GameMode (C++)

obstacle_*.lua
  └─> GameMode (C++)
```

--- 

## 마무리

이 아키텍처는 다음과 같은 이점을 제공합니다:

1. **빠른 개발**: Lua로 대부분의 로직 작성
2. **쉬운 수정**: Hot Reload로 즉시 테스트
3. **확장성**: 새로운 장애물/파워업을 Lua로 추가
4. **모드 지원**: 유저가 Lua 파일 수정 가능
5. **데이터 주도**: 모든 설정이 Lua 테이블
6. **시퀀스 제어**: 코루틴으로 복잡한 흐름 간단하게

런2 게임을 완성하는 데 필요한 모든 구조가 준비되었습니다!
