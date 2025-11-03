local _ENV = ...

local Queue = require("Queue");
local RandomManager = require("RandomManager");

-- lua는 클래스 개념이 없는 언어이므로 파일을 class 취급하여 작성하겠습니다.
-- local 키워드는 해당 파일에서만 접근할 수 있어 private처럼 활용합니다.

-- 대문자로 작성한 변수명은 상수로 정의합니다.
local DEFAULT_WIDTH = 10;
local DEFAULT_HEIGHT = 10;
local DEFAULT_DEPTH = 1;
local DEFAULT_MAP_SIZE = 20;
local DEFAULT_SCALE = 2.0;
local DEFAULT_FILLRATE = 0.70;
local DEFAULT_MAPSTART_INDEX = -5.0;

local Width = DEFAULT_WIDTH;
local Height = DEFAULT_HEIGHT;
local Depth = DEFAULT_DEPTH;
local Scale = DEFAULT_SCALE;
local MapSizeIndex = DEFAULT_MAP_SIZE;
local FillRate = DEFAULT_FILLRATE;
local MapStart = DEFAULT_MAPSTART_INDEX;

local CellChunks = {};
local MapChunksSpawned = {};
local CurrentMapId = 0;
local ChunkRemovalId = 0;

local STORAGE_POSITION = FTransform();
STORAGE_POSITION.Translation = FVector(-1000.0, 0.0, 0.0);
STORAGE_POSITION.Scale3D = FVector(Scale, Scale, Scale);
STORAGE_POSITION.Rotation = FQuat.MakeFromEuler(0, 0, 0);

local ChunkPool = Queue.new();

local function CreateCellChunk()
    local CellChunk = {};

    for i = 1, 4 do
        local Plane = {};
        for j = 1, Depth do
            local PlaneLow = {};
            for k = 1, Width do
                local Random = math.random();
                if (Random < FillRate) then
                    PlaneLow[k] = 1;
                else
                    PlaneLow[k] = 0;
                end
            end
            Plane[j] = PlaneLow;
        end
        CellChunk[i] = Plane;
    end

    return CellChunk;
end

local function CreateMapChunkWithCellChunk(CellChunk, XPosition)
    local MapChunk = {};

    -- 상단 (Top)
    local TopPlane = {};
    local Top = CellChunk[1];
    for i = 1, Depth do
        local Row = {};
        for j = 1, Width do
            if Top[i][j] > 0.01 then
                local Location = FVector(
                    XPosition + (i - 1) * Scale,
                    (j - 1 - (Width - 1) / 2.0) * Scale,
                    (Height / 2.0 + 0.5) * Scale
                );
                Row[j] = Queue.pop(ChunkPool);
                if Row[j] then
                    Row[j]:SetLocation(Location);
                    Row[j]:SetWallNormal(FVector(0, 0, -1));  -- 상단: 아래를 향함
                end
            else
                Row[j] = nil;
            end
        end
        TopPlane[i] = Row;
    end

    -- 하단 (Bottom)
    local BottomPlane = {};
    local Bottom = CellChunk[2];
    for i = 1, Depth do
        local Row = {};
        for j = 1, Width do
            if Bottom[i][j] > 0.01 then
                local Location = FVector(
                    XPosition + (i - 1) * Scale,
                    (j - 1 - (Width - 1) / 2.0) * Scale,
                    -(Height / 2.0 + 0.5) * Scale
                );
                Row[j] = Queue.pop(ChunkPool);
                if Row[j] then
                    Row[j]:SetLocation(Location);
                    Row[j]:SetWallNormal(FVector(0, 0, 1));  -- 하단: 위를 향함 (바닥)
                end
            else
                Row[j] = nil;
            end
        end
        BottomPlane[i] = Row;
    end

    -- 왼쪽 (Left)
    local LeftPlane = {};
    local Left = CellChunk[3];
    for i = 1, Depth do
        local Row = {};
        for j = 1, Height do
            if Left[i][j] > 0.01 then
                local Location = FVector(
                    XPosition + (i - 1) * Scale,
                    -(Width / 2.0 + 0.5) * Scale,
                    (j - 1 - (Height - 1) / 2.0) * Scale
                );
                Row[j] = Queue.pop(ChunkPool);
                if Row[j] then
                    Row[j]:SetLocation(Location);
                    Row[j]:SetWallNormal(FVector(0, 1, 0));  -- 왼쪽: 오른쪽(+Y)을 향함
                end
            else
                Row[j] = nil;
            end
        end
        LeftPlane[i] = Row;
    end

    -- 오른쪽 (Right)
    local RightPlane = {};
    local Right = CellChunk[4];  -- 수정: CellChunk[4] 사용
    for i = 1, Depth do
        local Row = {};
        for j = 1, Height do
            if Right[i][j] > 0.01 then
                local Location = FVector(
                    XPosition + (i - 1) * Scale,
                    (Width / 2.0 + 0.5) * Scale,
                    (j - 1 - (Height - 1) / 2.0) * Scale
                );
                Row[j] = Queue.pop(ChunkPool);
                if Row[j] then
                    Row[j]:SetLocation(Location);
                    Row[j]:SetWallNormal(FVector(0, -1, 0));  -- 오른쪽: 왼쪽(-Y)을 향함
                end
            else
                Row[j] = nil;
            end
        end
        RightPlane[i] = Row;
    end

    MapChunk.Top = TopPlane;
    MapChunk.Bottom = BottomPlane;
    MapChunk.Left = LeftPlane;
    MapChunk.Right = RightPlane;

    return MapChunk;
end

local function DeleteMapChunks(MapChunk)
    -- 상단 (Top), 하단 (Bottom)을 삭제
    for i = 1, Depth do
        for j = 1, Width do
            local TargetTop = MapChunk.Top[i][j];
            if TargetTop ~= nil then
                TargetTop:SetTransform(STORAGE_POSITION);
                Queue.push(ChunkPool, TargetTop);
            end
            local TargetBottom = MapChunk.Bottom[i][j];
            if TargetBottom ~= nil then
                TargetBottom:SetTransform(STORAGE_POSITION);
                Queue.push(ChunkPool, TargetBottom);
            end
        end
    end

    -- 왼쪽 (Left), 오른쪽 (Right)를 삭제
    for i = 1, Depth do
        for j = 1, Height do
            local TargetLeft = MapChunk.Left[i][j];
            if TargetLeft ~= nil then
                TargetLeft:SetTransform(STORAGE_POSITION);
                Queue.push(ChunkPool, TargetLeft);
            end
            local TargetRight = MapChunk.Right[i][j];
            if TargetRight ~= nil then
                TargetRight:SetTransform(STORAGE_POSITION);
                Queue.push(ChunkPool, TargetRight);
            end
        end
    end
end

local function InitializePool()
    local world = GEngine:GetPIEWorld();
    if world == nil then
        PrintToConsole("[MapGenerator] ERROR: PIE World is nil!");
        return;
    end

    -- Pool의 청크들은 시야에 보이지 않는 곳에 대기
    local PoolSize = (Width + Height) * Depth * 2 * MapSizeIndex;
    PrintToConsole("[MapGenerator] Creating pool with " .. PoolSize .. " actors");

    for i = 1, PoolSize do
        Queue.push(ChunkPool, world:SpawnActor(STORAGE_POSITION, "AGravityWall"));
    end

    PrintToConsole("[MapGenerator] Pool initialized. Queue size: " .. (ChunkPool.last - ChunkPool.first + 1));
end

local function InitializeMap()
    for i = 1, MapSizeIndex do
        CellChunks[i] = CreateCellChunk();
        MapChunksSpawned[i] = CreateMapChunkWithCellChunk(CellChunks[i], Depth * Scale * (i + MapStart - 1));
    end
end

local function Initialize()
    InitializePool();
    InitializeMap();
end

local function Update()
    local ActorLocation = MyActor:GetLocation();
    local Tmp = CurrentMapId;
    CurrentMapId = math.floor(ActorLocation.X / (Depth * Scale));

    -- 디버그: 플레이어 위치와 현재 청크 ID 출력 (매 프레임마다는 너무 많으니 청크 전환 시에만)
    if Tmp ~= CurrentMapId then
        -- 가장 오래된 청크 삭제
        local ChunkToDelete = MapChunksSpawned[ChunkRemovalId + 1];
        if ChunkToDelete == nil then
            -- PrintToConsole("[MapGenerator] ERROR: ChunkToDelete is nil at index " .. (ChunkRemovalId + 1));
        else
            -- PrintToConsole("[MapGenerator] Deleting chunk at index " .. (ChunkRemovalId + 1));
            DeleteMapChunks(ChunkToDelete);
        end

        -- 새 청크 생성 (플레이어 앞쪽에)
        local NewChunkXPosition = (CurrentMapId + MapStart + MapSizeIndex) * Depth * Scale;
        -- PrintToConsole("[MapGenerator] Creating new chunk at X: " .. NewChunkXPosition);
        
        local NewChunk = CreateCellChunk();
        CellChunks[ChunkRemovalId + 1] = NewChunk;
        MapChunksSpawned[ChunkRemovalId + 1] = CreateMapChunkWithCellChunk(NewChunk, NewChunkXPosition);

        -- 다음 삭제 대상 청크 인덱스 업데이트 (0, 1, 2 순환)
        ChunkRemovalId = (ChunkRemovalId + 1) % MapSizeIndex;
        -- PrintToConsole("[MapGenerator] New ChunkRemovalId: " .. ChunkRemovalId);
    end
end

-- Getter/Setter functions
function GetWidth()
    return Width;
end

function SetWidth(InWidth)
    Width = InWidth;
end

function GetHeight()
    return Height;
end

function SetHeight(InHeight)
    Height = InHeight;
end

function GetDepth()
    return Depth;
end

function SetDepth(InDepth)
    Depth = InDepth;
end

function GetScale()
    return Scale;
end

function SetScale(InScale)
    Scale = InScale;
end

-- Template functions
function BeginPlay()
    PrintToConsole("[MapGenerator] Begin Play");

    -- 랜덤 시드를 한 번만 설정 (모든 청크가 다른 패턴을 가지도록)
    RandomManager.SetNewRandomSeed();
    Initialize();
end

function EndPlay()
    PrintToConsole("[MapGenerator] End Play");
end

function OnOverlap(OverlappedComponent, OtherActor, OtherComp, ContactPoint, PenetrationDepth)
    -- No-op
end

function Tick(dt)
    Update();
end

-- 부활 작업
function Restart() end

