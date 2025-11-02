local _ENV = ...

-- lua는 클래스 개념이 없는 언어이므로 파일을 class 취급하여 작성하겠습니다.
-- local 키워드는 해당 파일에서만 접근할 수 있어 private처럼 활용합니다.

-- 대문자로 작성한 변수명은 상수로 정의합니다.
local DEFAULT_WIDTH = 5;
local DEFAULT_HEIGHT = 5;
local DEFAULT_DEPTH = 1;
local DEFAULT_MAP_SIZE = 30;
local DEFAULT_SCALE = 2.0;
local DEFAULT_FILLRATE = 0.5;

local Width = DEFAULT_WIDTH;
local Height = DEFAULT_HEIGHT;
local Depth = DEFAULT_DEPTH;
local Scale = DEFAULT_SCALE;
local MapSize = DEFAULT_MAP_SIZE;
local FillRate = DEFAULT_FILLRATE;

local CellChunks = {};
local MapChunks = {};
local CurrentMapId = 0;
local ChunkRemovalId = 0;

local function SetNewRandomSeed()
    local Seed = math.floor(os.clock() * 1000);  -- 정수로 변환
    math.randomseed(Seed);
end

local function CreateCellChunk()
    local CellChunk = {};

    -- 랜덤 시드는 청크당 한 번만 설정
    SetNewRandomSeed();

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
    local world = GEngine:GetPIEWorld();
    if world == nil then
        PrintToConsole("[MapGenerator] ERROR: PIE World is nil!");
        return {};
    end

    local MapChunk = {};

    -- 상단 (Top)
    local TopPlane = {};
    local Top = CellChunk[1];
    for i = 1, Depth do
        local Row = {};
        for j = 1, Width do
            if Top[i][j] > 0.01 then
                local transform = FTransform();
                transform.Translation = FVector(
                    XPosition + (i - 1) * Scale,
                    (j - 1 - (Width - 1) / 2.0) * Scale,
                    (Height / 2.0 + 0.5) * Scale
                );
                transform.Scale3D = FVector(Scale, Scale, Scale);
                transform.Rotation = FQuat.MakeFromEuler(0, 0, 0);
                Row[j] = world:SpawnActor(transform);
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
                local transform = FTransform();
                transform.Translation = FVector(
                    XPosition + (i - 1) * Scale,
                    (j - 1 - (Width - 1) / 2.0) * Scale,
                    -(Height / 2.0 + 0.5) * Scale
                );
                transform.Scale3D = FVector(Scale, Scale, Scale);
                transform.Rotation = FQuat.MakeFromEuler(0, 0, 0);
                Row[j] = world:SpawnActor(transform);
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
                local transform = FTransform();
                transform.Translation = FVector(
                    XPosition + (i - 1) * Scale,
                    -(Width / 2.0 + 0.5) * Scale,
                    (j - 1 - (Height - 1) / 2.0) * Scale
                );
                transform.Scale3D = FVector(Scale, Scale, Scale);
                transform.Rotation = FQuat.MakeFromEuler(0, 0, 0);
                Row[j] = world:SpawnActor(transform);
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
                local transform = FTransform();
                transform.Translation = FVector(
                    XPosition + (i - 1) * Scale,
                    (Width / 2.0 + 0.5) * Scale,
                    (j - 1 - (Height - 1) / 2.0) * Scale
                );
                transform.Scale3D = FVector(Scale, Scale, Scale);
                transform.Rotation = FQuat.MakeFromEuler(0, 0, 0);
                Row[j] = world:SpawnActor(transform);
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
    local world = GEngine:GetPIEWorld();
    if world == nil then
        PrintToConsole("[MapGenerator] ERROR: PIE World is nil!");
        return;
    end

    -- 상단 (Top), 하단 (Bottom)을 삭제
    for i = 1, Depth do
        for j = 1, Width do
            local TargetTop = MapChunk.Top[i][j];
            if TargetTop ~= nil then
                world:DestroyActor(TargetTop);
            end
            local TargetBottom = MapChunk.Bottom[i][j];
            if TargetBottom ~= nil then
                world:DestroyActor(TargetBottom);
            end
        end
    end

    -- 왼쪽 (Left), 오른쪽 (Right)를 삭제
    for i = 1, Depth do
        for j = 1, Height do
            local TargetLeft = MapChunk.Left[i][j];
            if TargetLeft ~= nil then
                world:DestroyActor(TargetLeft);
            end
            local TargetRight = MapChunk.Right[i][j];
            if TargetRight ~= nil then
                world:DestroyActor(TargetRight);
            end
        end
    end
end

local function Initialize()
    for i = 1, MapSize do
        CellChunks[i] = CreateCellChunk();
        MapChunks[i] = CreateMapChunkWithCellChunk(CreateCellChunk(), Depth * Scale * (i - 1));
    end
end

local function Update()
    -- MyActor 존재 확인
    if MyActor == nil then
        PrintToConsole("[MapGenerator] ERROR: MyActor is nil!");
        return;
    end

    local ActorLocation = MyActor:GetLocation();
    local Tmp = CurrentMapId;
    CurrentMapId = math.floor(ActorLocation.X / (Depth * Scale));

    -- 디버그: 플레이어 위치와 현재 청크 ID 출력 (매 프레임마다는 너무 많으니 청크 전환 시에만)
    if Tmp ~= CurrentMapId then
        -- 가장 오래된 청크 삭제
        local ChunkToDelete = MapChunks[ChunkRemovalId + 1];
        if ChunkToDelete == nil then
            PrintToConsole("[MapGenerator] ERROR: ChunkToDelete is nil at index " .. (ChunkRemovalId + 1));
        else
            -- PrintToConsole("[MapGenerator] Deleting chunk at index " .. (ChunkRemovalId + 1));
            DeleteMapChunks(ChunkToDelete);
        end

        -- 새 청크 생성 (플레이어 앞쪽에)
        local NewChunkXPosition = (CurrentMapId + MapSize) * Depth * Scale;
        -- PrintToConsole("[MapGenerator] Creating new chunk at X: " .. NewChunkXPosition);
        
        local NewChunk = CreateCellChunk();
        CellChunks[ChunkRemovalId + 1] = NewChunk;
        MapChunks[ChunkRemovalId + 1] = CreateMapChunkWithCellChunk(NewChunk, NewChunkXPosition);

        -- 다음 삭제 대상 청크 인덱스 업데이트 (0, 1, 2 순환)
        ChunkRemovalId = (ChunkRemovalId + 1) % MapSize;
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

    if MyActor == nil then
        PrintToConsole("[MapGenerator] ERROR: MyActor is nil in BeginPlay!");
    else
        local loc = MyActor:GetLocation();
        PrintToConsole("[MapGenerator] MyActor location: X=" .. loc.X .. ", Y=" .. loc.Y .. ", Z=" .. loc.Z);
    end

    PrintToConsole("[MapGenerator] Calling Initialize...");
    Initialize();
    PrintToConsole("[MapGenerator] Initialize complete. MapChunks count: " .. #MapChunks);
end

function EndPlay()
    PrintToConsole("[MapGenerator] End Play");
end

function OnOverlap(OverlappedComponent, OtherActor, OtherComp, ContactPoint, PenetrationDepth)
    -- No-op
end

function Tick(dt)
    local success, err = pcall(Update);
    if not success then
        PrintToConsole("[MapGenerator] ERROR in Tick: " .. tostring(err));
    end
end
