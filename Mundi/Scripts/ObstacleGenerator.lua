local _ENV = ...

local Queue = require("Queue");
local GlobalObjectManager = require("GlobalObjectManager");
local RandomManager = require("RandomManager");
local CollisionUtility = require("CollisionUtility");
local PlayerRotationUtility = require("PlayerRotationUtility");

local DEFAULT_HORIZONTAL_SPAWN_RANGE = 10 * 2;  -- 맵 기준 한 변의 블록 개수 * 블록 크기
local DEFAULT_VERTICAL_SPAWN_RANGE = 10 * 2;
local DEFAULT_SCALE = 0.4;

local DEFAULT_SPAWN_DELAY_MIN = 3.0;
local DEFAULT_SPAWN_DELAY_MAX = 5.0;

local DEFAULT_POOL_SIZE = 50;

local OBSTACLE_OBJ_FILE_PATH = "Data/Model/smokegrenade.obj";

local HorizontalSpawnRange = DEFAULT_HORIZONTAL_SPAWN_RANGE;
local VerticalSpawnRange = DEFAULT_VERTICAL_SPAWN_RANGE;
local Scale = DEFAULT_SCALE;

local SpawnDelayMin = DEFAULT_SPAWN_DELAY_MIN;
local SpawnDelayMax = DEFAULT_SPAWN_DELAY_MAX;

local ObstaclesSpawned = Queue.new();

local PoolSize = DEFAULT_POOL_SIZE;
local ObstaclePool = Queue.new();

local PRU = PlayerRotationUtility.New();
local ObstacleRotation = FVector(0.0, 0.0, 0.0);  -- BeginPlay에서 초기화됨

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

        Obstacle:AttachScript("Obstacle.lua");

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
    Obstacle:SetRotation(ObstacleRotation);
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

local function UpdateObstaclesRotation()
    local queueSize = ObstaclesSpawned.last - ObstaclesSpawned.first + 1;
    local MyActorRotation = MyActor:GetRotation();
    
    for i = 1, queueSize do
        local Obstacle = Queue.pop(ObstaclesSpawned);
        if (Obstacle ~= nil) then
            Obstacle:SetRotation(MyActorRotation);
        end
        Queue.push(ObstaclesSpawned, Obstacle);
    end
    ObstacleRotation = MyActorRotation;
end

-- Template functions
function BeginPlay()
    PrintToConsole("[ObstacleGenerator] Begin Play");
    RandomManager.SetNewRandomSeed();

    -- 풀 초기화
    InitializePool();
    
    -- 현재 회전값 저장
    PlayerRotationUtility.Initilaize(PRU, MyActor:GetRotation());

    -- 코루틴으로 장애물 스폰 시작 (한 번만 실행)
    StartCoroutine(function()
        SpawnNextObstacle();
    end);
end

function EndPlay()
    PrintToConsole("[ObstacleGenerator] End Play");
end

function OnOverlap(OverlappedComponent, OtherActor, OtherComp, ContactPoint, PenetrationDepth)
    -- PrintToConsole("[ObstacleGenerator] OnOverlap");
    if (CollisionUtility.IsObstacleActor(OtherActor)) then
        GetRunnerGameMode(GlobalObjectManager.GetPIEWorld()):OnPlayerDeath(MyActor);
    end
end

function Tick(dt)
    CheckObstacleLocationAndWithDraw();

    if PlayerRotationUtility.IsUpdated(PRU, MyActor:GetRotation()) then
        UpdateObstaclesRotation();
    end
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