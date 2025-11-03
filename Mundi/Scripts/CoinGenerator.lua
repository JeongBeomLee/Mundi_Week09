local _ENV = ...

local DEFAULT_HORIZONTAL_SPAWN_RANGE = 10 * 2;  -- 맵 기준 한 변의 블록 개수 * 블록 크기
local DEFAULT_VERTICAL_SPAWN_RANGE = 10 * 2;

local HorizontalSpawnRange = DEFAULT_HORIZONTAL_SPAWN_RANGE;
local VerticalSpawnRange = DEFAULT_VERTICAL_SPAWN_RANGE;

local DEFAULT_SPAWN_DELAY_MIN = 3.0;
local DEFAULT_SPAWN_DELAY_MAX = 5.0;

local SpawnDelayMin = DEFAULT_SPAWN_DELAY_MIN;
local SpawnDelayMax = DEFAULT_SPAWN_DELAY_MAX;

local STORAGE_POSITION = FTransform();
STORAGE_POSITION.Translation = FVector(-1000.0, 0.0, 0.0);
STORAGE_POSITION.Scale3D = FVector(0.05, 0.05, 0.05);
STORAGE_POSITION.Rotation = FQuat.MakeFromEuler(0, 0, 0);

local function GetCoinSpawnLocation()
    local CoinSpawnLocation = FVector(
            MyActor:GetLocation().X + (40.0 - 5.0) * 2, -- (MapSize + MapStart) * Scale
            math.random(-HorizontalSpawnRange / 2 + 2, HorizontalSpawnRange / 2 - 2.0), -- -MapWidth / 2.0 + Scale ...
            math.random(-VerticalSpawnRange / 2 + 2, VerticalSpawnRange / 2 - 2.0)
    );
    return CoinSpawnLocation;
end

local function SpawnCoin()
    local world = GEngine:GetPIEWorld();
    if world == nil then
        PrintToConsole("[MapGenerator] ERROR: PIE World is nil!");
        return;
    end
    
    local CoinLocation = GetCoinSpawnLocation();
    local coin = world:SpawnActor(STORAGE_POSITION, "ACoinActor");
    coin:SetLocation(CoinLocation);
    coin:SetRotation(FVector(0.0, 90.0, 0.0));
    coin:SetScale(FVector(0.05, 0.05, 0.05));
end

local function SpawnNextCoin()
    while (true) do
        SpawnCoin();
        coroutine.yield(math.random(SpawnDelayMin, SpawnDelayMax));
    end
end

-- BeginPlay: Actor가 생성되거나 레벨이 시작될 때 호출
function BeginPlay()
    local world = GEngine:GetPIEWorld();
    if world == nil then
        PrintToConsole("[MapGenerator] ERROR: PIE World is nil!");
        return;
    end

    StartCoroutine(function()
        SpawnNextCoin();
    end);
end

-- EndPlay: Actor가 제거되거나 레벨이 종료될 때 호출
function EndPlay()
    
end

-- OnOverlap: 다른 Actor와 충돌했을 때 호출
function OnOverlap(
    OverlappedComponent,
    OtherActor,
    OtherComp,
    ContactPoint,
    PenetrationDepth
)
    
end

-- Tick: 매 프레임마다 호출 (dt: 델타 타임)
function Tick(dt)
    
end

function Restart() end
