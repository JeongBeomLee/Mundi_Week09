local _ENV = ...

local Queue = require("Queue");
local GlobalObjectManager = require("GlobalObjectManager");
local RandomManager = require("RandomManager");
local CollisionUtility = require("CollisionUtility");

local DEFAULT_HORIZONTAL_SPAWN_RANGE = 10 * 2;  -- 맵 기준 한 변의 블록 개수 * 블록 크기
local DEFAULT_VERTICAL_SPAWN_RANGE = 10 * 2;
local DEFAULT_SCALE = 0.4;

local DEFAULT_SPAWN_DELAY_MIN = 3.0;
local DEFAULT_SPAWN_DELAY_MAX = 5.0;

local DEFAULT_POOL_SIZE = 50;

local OBSTACLE_OBJ_FILE_PATH = "Data/Model/smokegrenade.obj";

local BILLBOARD_COUNT = 5;  -- 빌보드 컴포넌트 개수

local HorizontalSpawnRange = DEFAULT_HORIZONTAL_SPAWN_RANGE;
local VerticalSpawnRange = DEFAULT_VERTICAL_SPAWN_RANGE;
local Scale = DEFAULT_SCALE;

local SpawnDelayMin = DEFAULT_SPAWN_DELAY_MIN;
local SpawnDelayMax = DEFAULT_SPAWN_DELAY_MAX;

local ObstaclesSpawned = Queue.new();

local PoolSize = DEFAULT_POOL_SIZE;
local ObstaclePool = Queue.new();

-- PlayerController와 CameraManager 저장
local PlayerController = nil;
local PlayerCameraManager = nil;
local ActiveCameraShakeModifiers = {};  -- 활성 CameraShake 추적용

-- 충돌 컴포넌트가 포함되어 있으면 다른 물체와 OnOVerlap될 수 있으므로
-- Pool마다 STORAGE_POSITION을 다르게 해야 함.
local STORAGE_POSITION = FTransform();
STORAGE_POSITION.Translation = FVector(-5000.0, -100.0, 0.0);
STORAGE_POSITION.Scale3D = FVector(Scale, Scale, Scale);
STORAGE_POSITION.Rotation = FQuat.MakeFromEuler(0, 0, 0);

local function InitializePool()
    local PieWorld = GlobalObjectManager.GetPIEWorld();

    for i = 1, PoolSize do
        local Obstacle = PieWorld:SpawnActor(STORAGE_POSITION);
        local StaticMeshComponent = Obstacle:GetStaticMeshComponent();
        StaticMeshComponent:SetStaticMesh(OBSTACLE_OBJ_FILE_PATH);

        local CapsuleComponent = Obstacle:CreateCapsuleComponent("CollisionComponent");
        CapsuleComponent:SetCapsuleSize(4.0, 5.5, true);
        CapsuleComponent:SetRelativeLocation(FVector(0.0, 3.0, 0.5));

        -- PrintToConsole("[ObstacleGenerator] Obstacle CapsuleComponent created.");

        -- -- 빌보드 컴포넌트를 여러 개 생성
        -- local centerY = 2.5
        -- local spacing = 2.5

        -- -- 빌보드 개수가 홀수/짝수일 때 중앙 정렬 계산
        -- local startOffset = -(BILLBOARD_COUNT - 1) * spacing / 2

        -- for j = 1, BILLBOARD_COUNT do
        --     local yOffset = startOffset + (j - 1) * spacing
        --     local BillboardComponent = Obstacle:CreateBillboardComponent("BillboardComponent" .. j);
        --     BillboardComponent:SetTextureName("Data/UI/Icons/Pawn_64x.png");
        --     BillboardComponent:SetRelativeLocation(FVector(-5.0, centerY + yOffset, 4.0));
            
        --     PrintToConsole("[ObstacleGenerator] Obstacle BillboardComponent " .. j .. " created at Y: " .. (centerY + yOffset));
        -- end

        Obstacle:AttachScript("Obstacle.lua");

        -- PrintToConsole("[ObstacleGenerator] Obstacle script attached.");

        Queue.push(ObstaclePool, Obstacle);
    end
end

local function GetObstacleSpawnLocation()
    local ObstacleSpawnLocation = FVector(
            MyActor:GetLocation().X + (40.0 - 5.0) * 2, -- (MapSize + MapStart) * Scale
            math.random(-HorizontalSpawnRange / 2 + 2, HorizontalSpawnRange / 2 - 2.0), -- -MapWidth / 2.0 + Scale ...
            math.random(-VerticalSpawnRange / 2 + 2, VerticalSpawnRange / 2 - 2.0)
    );
    return ObstacleSpawnLocation;
end

local function SpawnObstacle()
    local Obstacle = Queue.pop(ObstaclePool);
    if (Obstacle == nil) then
        PrintToConsole("[ObstacleGenerator] WARNING: ObstaclePool is empty!");
        return;
    end
    local ObstacleLocation = GetObstacleSpawnLocation();
    Obstacle:SetLocation(ObstacleLocation);
    Queue.push(ObstaclesSpawned, Obstacle);
end

local function SpawnNextObstacle()
    while (true) do
        if IsGamePlaying(GlobalObjectManager.GetPIEWorld()) then
            SpawnObstacle();
        end
        coroutine.yield(math.random(SpawnDelayMin, SpawnDelayMax));
    end
end

local function CheckObstacleLocationAndWithDraw()
    local Oldest = Queue.head(ObstaclesSpawned);
    if (Oldest == nil) then
        return;
    end

    if (Oldest:GetLocation().X < MyActor:GetLocation().X - 5.0) then
        Oldest:SetTransform(STORAGE_POSITION);
        Queue.pop(ObstaclesSpawned);
        Queue.push(ObstaclePool, Oldest);  -- ObstaclePool로 반환
    end
end

-- Template functions
function BeginPlay()
    PrintToConsole("[ObstacleGenerator] Begin Play");
    RandomManager.SetNewRandomSeed();

    -- 풀 초기화
    InitializePool();

    -- PlayerController와 PlayerCameraManager 가져오기
    local GameMode = GetRunnerGameMode(GlobalObjectManager.GetPIEWorld());
    if GameMode then
        PlayerController = GameMode:GetPlayerController();
        if PlayerController then
            PlayerCameraManager = PlayerController:GetPlayerCameraManager();
            if PlayerCameraManager then
                PrintToConsole("[ObstacleGenerator] PlayerCameraManager initialized");
            else
                PrintToConsole("[ObstacleGenerator] WARNING: PlayerCameraManager is nil");
            end
        else
            PrintToConsole("[ObstacleGenerator] WARNING: PlayerController is nil");
        end
    else
        PrintToConsole("[ObstacleGenerator] WARNING: GameMode is nil");
    end

    -- 코루틴으로 장애물 스폰 시작 (한 번만 실행)
    StartCoroutine(function()
        SpawnNextObstacle();
    end);
end

function EndPlay()
    PrintToConsole("[ObstacleGenerator] End Play");
end

local function AddCameraShake()
    if not PlayerCameraManager then
        return;
    end

    -- LetterBox 모디파이어 생성
    PrintToConsole("[RunnerCharacter] Adding LetterBox camera modifier")
    local letterBox = UCameraModifier_LetterBox()
    -- PlayerCameraManager에 추가
    local cameraManager = GameMode:GetPlayerController():GetPlayerCameraManager()
    -- 레터박스 시작 (크기: 0.15, 불투명도: 1.0, 페이드인 시간: 1초)
    letterBox:StartLetterBox(0.15, 1.0, 1.0)
    cameraManager:AddCameraModifier(letterBox)
    PrintToConsole("[RunnerCharacter] LetterBox camera modifier added")

    -- local CameraShakeModifier = UCameraModifier_CameraShake();
    -- -- 흔들리는 정도 설정
    -- CameraShakeModifier:SetRotationAmplitude(10.0);
    -- -- 흔들리는 시간 설정
    -- CameraShakeModifier:SetAlphaInTime(2.0);
    -- -- 흔들림 곡선의 주기 설정
    -- CameraShakeModifier:SetNumSamples(6);
    -- -- 앞선 설정으로 새로운 흔들림 생성
    -- CameraShakeModifier:GetNewShake();

    -- PlayerCameraManager:AddCameraModifier(CameraShakeModifier);
    -- table.insert(ActiveCameraShakeModifiers, CameraShakeModifier);
end

function OnOverlap(OverlappedComponent, OtherActor, OtherComp, ContactPoint, PenetrationDepth)
    if not CollisionUtility.IsObstacleActor(OtherActor) then
        return;
    end

    -- 플레이어 사망 처리
    GetRunnerGameMode(GlobalObjectManager.GetPIEWorld()):OnPlayerDeath(MyActor);

    -- 카메라 셰이크 추가
    AddCameraShake();
end

local function RemoveDisabledCameraShakes()
    local i = 1;
    while i <= #ActiveCameraShakeModifiers do
        local modifier = ActiveCameraShakeModifiers[i];

        if modifier:IsDisabled() then
            PlayerCameraManager:RemoveCameraModifier(modifier);
            table.remove(ActiveCameraShakeModifiers, i);
        else
            i = i + 1;
        end
    end
end

function Tick(dt)
    CheckObstacleLocationAndWithDraw();

    -- 카메라 업데이트
    if PlayerCameraManager then
        PlayerCameraManager:UpdateCamera(dt);
    end

    -- 비활성화된 카메라 셰이크 제거
    RemoveDisabledCameraShakes();
end

-- 부활 작업
function Restart()
    while not Queue.isEmpty(ObstaclesSpawned) do
        local Obstacle = Queue.pop(ObstaclesSpawned);
        if Obstacle ~= nil then
            Obstacle:SetTransform(STORAGE_POSITION);
            Queue.push(ObstaclePool);
        end
    end
end