#include "pch.h"
#include "Source/Runtime/LuaScripting/UScriptManager.h"

IMPLEMENT_CLASS(UScriptManager)

UScriptManager::UScriptManager()
{
    Initialize();
}

UScriptManager::~UScriptManager()
{
}

void UScriptManager::Initialize()
{
    /*
     * Lua Script에서 별도로 Library를 include하지 않아도 되도록
     * 전역으로 Include하는 설정
     */
    Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    // 전역으로 lua에 타입을 등록

    // 전역으로 lua에 값을 등록

    // 경로에 있는 모든 lua 파일들을 모두 environment화하여 cache
    for (auto& entry : fs::directory_iterator(ScriptsFilePath))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".lua") continue;

        FString Path = entry.path().string();
        FString FileName = entry.path().filename().string();

        try {
            // 새 environment 생성 (globals 기반)
            sol::environment Env(Lua, sol::create, Lua.globals());

            // 스크립트 실행
            sol::load_result scriptLoad = Lua.load_file(Path);
            if (!scriptLoad.valid()) {
                sol::error err = scriptLoad;
                FString ErrorMessage = "[Script Manager] Lua Load Error" + FileName + ": " + err.what();
                UE_LOG(ErrorMessage.c_str());
                continue;
            }

            sol::protected_function_result result = scriptLoad(Env);
            if (!result.valid()) {
                sol::error err = result;
                FString ErrorMessage = "[Script Manager] Lua Runtime Error" + FileName + ": " + err.what();
                UE_LOG(ErrorMessage.c_str());
                continue;
            }

            // table 반환 여부 확인
            sol::table ScriptTable;
            sol::object returned = Env["_G"]; // 혹은 스크립트가 return table이면 result 반환값 사용 가능
            if (returned.is<sol::table>()) {
                ScriptTable = returned.as<sol::table>();
            }
            
            FLuaTemplateFunctions LuaTemplateFunction =
                GetTemplateFunctionFromScript(Env);

            // 맵에 저장
            ScriptsByName[FileName] = new FScript{
                FileName,
                Env,
                ScriptTable,
                FLuaTemplateFunctions()
            };

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

UScriptManager& UScriptManager::GetInstance()
{
    static UScriptManager Instance;
    return Instance;
}

/* Private */

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