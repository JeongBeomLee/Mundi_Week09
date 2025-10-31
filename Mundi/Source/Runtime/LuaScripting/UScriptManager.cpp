#include "pch.h"
#include "Source/Runtime/LuaScripting/UScriptManager.h"
#include "Source/Runtime/Engine/Components/SceneComponent.h"
#include "Source/Runtime/LuaScripting/ScriptGlobalFunction.h"

IMPLEMENT_CLASS(UScriptManager)

UScriptManager::UScriptManager()
{
    Initialize();
}

UScriptManager::~UScriptManager()
{
    Shutdown();
}

void UScriptManager::AttachScriptTo(AActor* Target, FString ScriptName)
{
    // 이미 같은 스크립트가 부착되어 있으면 return
    for (const TPair<AActor* const, TArray<FScript*>>& Script : ScriptsByOwner)
    {
        if (Target == Script.first)
        {
            for (FScript* ScriptData : Script.second)
            {
                if (ScriptData && ScriptData->ScriptName == ScriptName)
                    return;
            }
        }
    }

    ScriptsByOwner[Target].push_back(GetOrCreate(ScriptName));
}

TMap<AActor*, TArray<FScript*>>& UScriptManager::GetScriptsByOwner()
{
    return ScriptsByOwner;
}

/* public static */
UScriptManager& UScriptManager::GetInstance()
{
    static UScriptManager Instance;
    return Instance;
}

/* Private */
void UScriptManager::Initialize()
{
    /*
     * Lua Script에서 별도로 Library를 include하지 않아도 되도록
     * 전역으로 Include하는 설정
     */
    Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    RegisterUserTypeToLua();
    
    // 전역으로 lua에 값을 등록

    // 경로에 있는 모든 lua 파일들을 모두 environment화하여 cache
    for (const fs::directory_entry& entry : fs::directory_iterator(SCRIPT_FILE_PATH))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".lua") continue;
        
        FString FileName = entry.path().filename().string();

        try {
            // 맵에 저장
            ScriptsByName[FileName] = GetOrCreate(FileName);

            FString SuccessMessage =
                FString("[Script Manager] Script ") + FileName + " Loaded Successfully.";
            UE_LOG(SuccessMessage.c_str());
        }
        catch (const std::exception& e) {
            FString ErrorMessage =
                FString("[Script Manager] Exception : ") + FileName + ": " + e.what();
            UE_LOG(ErrorMessage.c_str());
        }
    }
}

void UScriptManager::Shutdown()
{
    lua_close(Lua);
}

/* 전역으로 lua에 타입을 등록 */
void UScriptManager::RegisterUserTypeToLua()
{
    // FName 등록
    sol::usertype<FName> NameType = Lua.new_usertype<FName>(
        "FName",
        sol::constructors<FName(), FName(const char*), FName(const FString&)>());
    NameType["ToString"] = &FName::ToString;

    // USceneComponent 등록
    sol::usertype<USceneComponent> SceneComponentType = Lua.new_usertype<USceneComponent>(
        "USceneComponent",
        sol::constructors<USceneComponent()>());
    SceneComponentType["GetSceneId"] = &USceneComponent::GetSceneId;
    
    // AActor 등록
    sol::usertype<AActor> ActorType = Lua.new_usertype<AActor>(
        "AActor",
        sol::constructors<AActor()>());
    ActorType["Name"] = &AActor::Name;
    ActorType["GetSceneComponents"] = &AActor::GetSceneComponents;
}

void UScriptManager::RegisterGlobalFuncToLua()
{
    Lua["print"] = PrintToConsole;
}

void UScriptManager::RegisterActorToLua(sol::environment InEnv, AActor* InActor)
{
    InEnv["MyActor"] = *InActor;
}

// Lua로부터 Template 함수를 가져온다.
// 해당 함수가 없으면 Throw한다.
FLuaTemplateFunctions UScriptManager::GetTemplateFunctionFromScript(
    sol::environment& InEnv
    )
{
    FLuaTemplateFunctions LuaTemplateFunctions;

    auto AssignFunction = [&InEnv](
        sol::function& Target,
        const char* FunctionName
    )
    {
        Target = InEnv[FunctionName];
        if (!Target.valid())
            throw std::runtime_error(FString("Missing Lua function: ") + FunctionName);
    };

    AssignFunction(LuaTemplateFunctions.BeginPlay, "BeginPlay");
    AssignFunction(LuaTemplateFunctions.EndPlay, "EndPlay");
    AssignFunction(LuaTemplateFunctions.OnOverlap, "OnOverlap");
    AssignFunction(LuaTemplateFunctions.Tick, "Tick");
}

FScript* UScriptManager::GetOrCreate(FString InPath)
{
    fs::path Path(SCRIPT_FILE_PATH + InPath);
    FString FileName = Path.filename().string();

    // 캐시에 존재하면 반환
    if (!(ScriptsByName.find(FileName) == ScriptsByName.end()))
    {
        return ScriptsByName[FileName];
    }

    /*
     *  경로에 파일이 존재하지 않으면
     *  template.lua를 복제하여 스크립트를 생성한다.
     */
    if (!fs::exists(Path))
    {
        Path = fs::path(FileName);

        fs::copy_file(DEFAULT_FILE_PATH, Path, fs::copy_options::overwrite_existing);
    }
    
    // 새 environment 생성 (globals 기반)
    sol::environment Env(Lua, sol::create, Lua.globals());

    // 스크립트 실행
    sol::load_result scriptLoad = Lua.load_file(Path.string());
    if (!scriptLoad.valid()) {
        sol::error Err = scriptLoad;
        throw Err;
    }

    sol::protected_function_result result = scriptLoad(Env);
    if (!result.valid()) {
        sol::error Err = result;
        throw Err;
    }

    // table 반환 여부 확인
    sol::table ScriptTable;
    // 혹은 스크립트가 return table이면 result 반환값 사용 가능
    sol::object returned = Env["_G"];
    if (returned.is<sol::table>()) {
        ScriptTable = returned.as<sol::table>();
    }

    // LuaTemplateFunction 생성
    FLuaTemplateFunctions LuaTemplateFunction =
        GetTemplateFunctionFromScript(Env);

    FScript* NewScript = new FScript;

    NewScript->ScriptName = FileName;
    NewScript->Env = Env;
    NewScript->Table = ScriptTable;
    NewScript->LuaTemplateFunctions = LuaTemplateFunction;

    return NewScript;
}
