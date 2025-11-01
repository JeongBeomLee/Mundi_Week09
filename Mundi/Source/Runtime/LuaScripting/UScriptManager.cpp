#include "pch.h"
#include "Source/Runtime/LuaScripting/UScriptManager.h"

#include "CameraActor.h"
#include "CollisionComponent/ShapeComponent.h"
#include "Source/Runtime/Core/Object/Actor.h"
#include "Source/Runtime/Engine/Components/SceneComponent.h"
#include "Source/Runtime/LuaScripting/ScriptGlobalFunction.h"

IMPLEMENT_CLASS(UScriptManager)

UScriptManager::UScriptManager()
{
    Initialize();
}

UScriptManager::~UScriptManager()
{
    // Shutdown();
}

void UScriptManager::AttachScriptTo(FLuaLocalValue LuaLocalValue, const FString& ScriptName)
{
    // 이미 같은 스크립트가 부착되어 있으면 return
    for (const TPair<AActor* const, TArray<FScript*>>& Script : ScriptsByOwner)
    {
        if (LuaLocalValue.MyActor == Script.first)
        {
            for (FScript* ScriptData : Script.second)
            {
                if (ScriptData && ScriptData->ScriptName == ScriptName)
                {
                    UE_LOG("[Script Manager] Actor alreay have %s", ScriptName.c_str());
                    return;
                }
            }
        }
    }

    try
    {
        FScript* Script = GetOrCreate(ScriptName);
        RegisterLocalValueToLua(Script->Env, LuaLocalValue);
        ScriptsByOwner[LuaLocalValue.MyActor].push_back(Script);
        LinkOnOverlapWithShapeComponent(
            LuaLocalValue.MyActor,
            Script->LuaTemplateFunctions.OnOverlap
        );
    }
    catch (std::exception& e)
    {
        FString ErrorMessage = FString("[Script Manager] ") + ScriptName + " creation failed : " + e.what();
        UE_LOG(ErrorMessage.c_str());
        // 예외를 다시 던져서 호출자가 처리할 수 있도록 함
        throw;
    }
}

void UScriptManager::PrintDebugLog()
{
    // 이미 같은 스크립트가 부착되어 있으면 return
    for (const TPair<AActor* const, TArray<FScript*>>& Script : ScriptsByOwner)
    {
        for (FScript* ScriptData : Script.second)
        {
            UE_LOG(
                "[Script Manager] Debug Log - Owner %s, Script %s",
                Script.first->GetName().ToString(),
                ScriptData->ScriptName.c_str()
            );
        }
    }
}

void UScriptManager::DetachScriptFrom(AActor* InActor, const FString& ScriptName)
{
    // Actor에 부착된 Script를 찾는다.
    for (TPair<AActor* const, TArray<FScript*>>& Script : ScriptsByOwner)
    {
        if (InActor == Script.first)
        {
            for (auto Iter = Script.second.begin(); Iter != Script.second.end(); Iter++)
            {
                if (*Iter && (*Iter)->ScriptName == ScriptName)
                {
                    FScript* Tmp = *Iter;
                    Script.second.erase(Iter);

                    delete Tmp;
                    break;
                }
            }
        }
    }
}

void UScriptManager::DetachAllScriptFrom(AActor* InActor)
{
    for (TPair<AActor* const, TArray<FScript*>>& Script : ScriptsByOwner)
    {
        if (InActor == Script.first)
        {
            while (!Script.second.empty())
            {
                FScript* Tmp = Script.second.back();
                Script.second.RemoveAt(Script.second.size() - 1);
                delete Tmp;
            }
        }
    }
}

TMap<AActor*, TArray<FScript*>>& UScriptManager::GetScriptsByOwner()
{
    return ScriptsByOwner;
}

TArray<FScript*> UScriptManager::GetScriptsOfActor(AActor* InActor)
{
    auto Found = ScriptsByOwner.find(InActor);
    // Cache에 Actor가 등록이 되어있지 않으면 빈 배열을 반환한다.
    if (Found == ScriptsByOwner.end())
        return {};

    return Found->second;
}

void UScriptManager::CheckAndHotReloadLuaScript()
{
    for (auto& ScriptPair : ScriptsByOwner)
    {
        for (FScript* Script : ScriptPair.second)
        {
            // 파일이 존재하지 않을 경우
            if (!fs::exists(Script->ScriptName))
            {
                continue;
            }

            auto CurrentWriteTime = std::filesystem::last_write_time(Script->ScriptName);
            if (CurrentWriteTime > Script->LastModifiedTime)
            {
                FString HotReloadMessage =
                    FString("[Script Manager] Lua Script: ") +
                    Script->ScriptName +
                    " hot reload.";
                UE_LOG(HotReloadMessage.c_str());

                // 기존 상태 백업
                sol::environment OldEnv = Script->Env;
                sol::table OldTable = Script->Table;
                FLuaTemplateFunctions OldFuncs = Script->LuaTemplateFunctions;

                try
                {
                    fs::path Path(SCRIPT_FILE_PATH + Script->ScriptName);
                    SetLuaScriptField(
                        Path,
                        Script->Env,
                        Script->Table,
                        Script->LuaTemplateFunctions
                    );
                    RegisterLocalValueToLua(Script->Env, Script->LuaLocalValue);
                }
                catch (std::exception& e)
                {
                    FString ErrorMessage =
                        FString("[Script Manager] Lua Script: ") +
                        Script->ScriptName +
                        " hot reload failed and fall backed.";

                    Script->Env = OldEnv;
                    Script->Table = OldTable;
                    Script->LuaTemplateFunctions = OldFuncs;
                }

                Script->LastModifiedTime = CurrentWriteTime;
            }
        }
    }
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
    Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::coroutine);

    RegisterUserTypeToLua();
    RegisterGlobalFuncToLua();
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
        sol::no_constructor);
    NameType["ToString"] = &FName::ToString;
    
    // USceneComponent 등록
    sol::usertype<USceneComponent> SceneComponentType = Lua.new_usertype<USceneComponent>(
        "USceneComponent",
        sol::constructors<USceneComponent()>());
    SceneComponentType["GetSceneId"] = &USceneComponent::GetSceneId;

    // UShapeComponent 등록 (충돌 컴포넌트)
    Lua.new_usertype<UShapeComponent>("UShapeComponent",
        sol::base_classes, sol::bases<USceneComponent, UActorComponent>()
    );

    // FVector 타입을 Lua에 등록
    Lua.new_usertype<FVector>("FVector",
        sol::constructors<FVector(), FVector(float, float, float)>(),
        "X", &FVector::X,
        "Y", &FVector::Y,
        "Z", &FVector::Z,
        // 연산자 오버로딩 (메타메서드)
        sol::meta_function::addition, [](const FVector& a, const FVector& b) { return a + b; },
        sol::meta_function::subtraction, [](const FVector& a, const FVector& b) { return a - b; },
        sol::meta_function::multiplication, [](const FVector& v, float scalar) { return v * scalar; },
        sol::meta_function::division, [](const FVector& v, float scalar) { return v / scalar; },
        sol::meta_function::unary_minus, [](const FVector& v) { return -v; },
        "Add", [](const FVector& a, const FVector& b) { return a + b; },
        "Sub", [](const FVector& a, const FVector& b) { return a - b; },
        "Mul", [](const FVector& v, float scalar) { return v * scalar; }
    );

    // FQuat 타입을 Lua에 등록
    Lua.new_usertype<FQuat>("FQuat",
        sol::constructors<FQuat()>(),
        "MakeFromEuler", [](float pitch, float yaw, float roll) {
            return FQuat::MakeFromEulerZYX(FVector(pitch, yaw, roll));
        }
    );

    // Actor 래퍼 클래스 등록
    Lua.new_usertype<AActor>("AActor",
        "GetLocation", &AActor::GetActorLocation,
        "SetLocation", &AActor::SetActorLocation,
        "GetRotation", &AActor::GetActorRotation,
        "SetRotation", sol::overload(
            static_cast<void(AActor::*)(const FVector&)>(&AActor::SetActorRotation),
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::SetActorRotation)
        ),
        "GetScale", &AActor::GetActorScale,
        "SetScale", &AActor::SetActorScale,
        "AddWorldLocation", &AActor::AddActorWorldLocation,
        "AddWorldRotation", sol::overload(
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::AddActorWorldRotation)
        ),
        "GetName", &AActor::GetName
    );

    //ActorType["GetSceneComponents"] = &AActor::GetSceneComponents;
}

void UScriptManager::RegisterGlobalFuncToLua()
{
    Lua["PrintToConsole"] = PrintToConsole;
	CoroutineScheduler.RegisterCoroutineTo(Lua);
}

void UScriptManager::RegisterLocalValueToLua(sol::environment& InEnv, FLuaLocalValue LuaLocalValue)
{
    InEnv["MyActor"] = LuaLocalValue.MyActor;
}

// 스크립트를 Actor에 부착할 때 Actor의 ShapeComponent에 Lua의 OnOverlap 함수를 연결한다
void UScriptManager::LinkOnOverlapWithShapeComponent(AActor* MyActor, sol::function OnOverlap)
{
    for (UActorComponent* ActorComponent : MyActor->GetOwnedComponents())
    {
        UShapeComponent* ShapeComponent = Cast<UShapeComponent>(ActorComponent);
        if (ShapeComponent)
        {
            auto DelegateHandle = ShapeComponent->OnComponentBeginOverlap.Add(
            OnOverlap
            );
        }
    }
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

    return LuaTemplateFunctions;
}

void UScriptManager::SetLuaScriptField(
    fs::path Path,
    sol::environment& InEnv,
    sol::table& InScriptTable,
    FLuaTemplateFunctions& InLuaTemplateFunction
)
{
    // 새 environment 생성 (globals 기반)
    InEnv = sol::environment (Lua, sol::create, Lua.globals());

    // 스크립트 실행
    sol::load_result scriptLoad = Lua.load_file(Path.string());
    if (!scriptLoad.valid()) {
        sol::error Err = scriptLoad;
        throw Err;
    }

    sol::protected_function_result result = scriptLoad(InEnv);
    if (!result.valid()) {
        sol::error Err = result;
        throw Err;
    }
    
    // 혹은 스크립트가 return table이면 result 반환값 사용 가능
    sol::object returned = InEnv["_G"];
    if (returned.is<sol::table>()) {
        InScriptTable = returned.as<sol::table>();
    }

    // LuaTemplateFunction 생성
    InLuaTemplateFunction =
        GetTemplateFunctionFromScript(InEnv);
}

FScript* UScriptManager::GetOrCreate(FString InScriptName)
{
    fs::path Path(SCRIPT_FILE_PATH + InScriptName);
    FString ScriptName = Path.filename().string();

    // 파일이 존재하지 않으면 예외 발생
    if (!fs::exists(Path))
    {
        throw std::runtime_error(FString("Script file not found: ") + ScriptName +
                                 ". Please create the script file first.");
    }

    sol::environment Env;
    sol::table Table;
    FLuaTemplateFunctions LuaTemplateFunctions;

    SetLuaScriptField(Path, Env, Table, LuaTemplateFunctions);

    FScript* NewScript = new FScript;

    NewScript->ScriptName = ScriptName;
    NewScript->Env = Env;
    NewScript->Table = Table;
    NewScript->LuaTemplateFunctions = LuaTemplateFunctions;

    // Hot reload를 위해 초기 수정 시간 설정
    if (fs::exists(Path))
    {
        NewScript->LastModifiedTime = fs::last_write_time(Path);
    }

    return NewScript;
}

// 스크립트 파일 생성 (template.lua 복사)
bool UScriptManager::CreateScriptFile(const FString& ScriptName)
{
    try
    {
        fs::path TargetPath(SCRIPT_FILE_PATH + ScriptName);

        // 이미 파일이 존재하면 false 반환
        if (fs::exists(TargetPath))
        {
            UE_LOG("[Script Manager] Script file already exists: %s", ScriptName.c_str());
            return false;
        }

        // Scripts 디렉토리가 없으면 생성
        fs::path ScriptDir(SCRIPT_FILE_PATH);
        if (!fs::exists(ScriptDir))
        {
            fs::create_directories(ScriptDir);
        }

        // template.lua 복사
        fs::copy_file(DEFAULT_FILE_PATH, TargetPath, fs::copy_options::overwrite_existing);

        UE_LOG("[Script Manager] Created new script file: %s", ScriptName.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        UE_LOG("[Script Manager] Failed to create script file: %s - Error: %s",
               ScriptName.c_str(), e.what());
        return false;
    }
}

// 스크립트 파일을 외부 에디터로 열기
bool UScriptManager::OpenScriptInEditor(const FString& ScriptName)
{
    try
    {
        fs::path ScriptPath(SCRIPT_FILE_PATH + ScriptName);

        // 파일이 존재하는지 확인
        if (!fs::exists(ScriptPath))
        {
            UE_LOG("[Script Manager] Script file not found: %s", ScriptName.c_str());
            return false;
        }

        // 절대 경로로 변환
        fs::path AbsolutePath = fs::absolute(ScriptPath);
        FString PathStr = AbsolutePath.string();

        // Windows에서 기본 텍스트 에디터로 열기
#ifdef _WIN32
        HINSTANCE Result = ShellExecuteA(
            nullptr,              // hwnd
            "open",               // operation
            PathStr.c_str(),      // file
            nullptr,              // parameters
            nullptr,              // directory
            SW_SHOW               // show command
        );

        // ShellExecute는 성공 시 32보다 큰 값을 반환
        if ((INT_PTR)Result > 32)
        {
            UE_LOG("[Script Manager] Opened script in editor: %s", ScriptName.c_str());
            return true;
        }
        else
        {
            UE_LOG("[Script Manager] Failed to open script in editor: %s", ScriptName.c_str());
            return false;
        }
#else
        // Linux/Mac의 경우 시스템 명령으로 처리
        FString Command = "xdg-open \"" + PathStr + "\"";
        int Result = system(Command.c_str());
        return Result == 0;
#endif
    }
    catch (const std::exception& e)
    {
        UE_LOG("[Script Manager] Exception while opening script: %s - Error: %s",
               ScriptName.c_str(), e.what());
        return false;
    }
}
