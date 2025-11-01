#pragma once
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

struct FLuaTemplateFunctions
{
    sol::function BeginPlay;
    sol::function EndPlay;
    sol::function OnOverlap;
    sol::function Tick;
};

struct FLuaLocalValue
{
    AActor* MyActor = nullptr;
};

struct FScript
{
    FString ScriptName;

    sol::environment Env;
    sol::table Table;
    FLuaTemplateFunctions LuaTemplateFunctions;

    // hot reload support
    FLuaLocalValue LuaLocalValue;
    fs::file_time_type LastModifiedTime;
};

class UScriptManager : public UObject
{
    DECLARE_CLASS(UScriptManager, UObject);
public:
    /*
     Single Ton Pattern
     이라 원래 Private에 있어야 하지만 IMPLEMENT_CLASS 이슈로
     일단 public에 넣습니다.
     TODO: 추후 수정해야 함.
    */
    UScriptManager();
    ~UScriptManager() override;

    void AttachScriptTo(FLuaLocalValue LuaLocalValue, FString ScriptName);

    TMap<AActor*, TArray<FScript*>>& GetScriptsByOwner();
    
    void CheckAndHotReloadLuaScript();
public:
    static UScriptManager& GetInstance();
private:
    void Initialize();
    void Shutdown();

    void RegisterUserTypeToLua();
    void RegisterGlobalFuncToLua();
    
    void RegisterLocalValueToLua(
        sol::environment& InEnv,
        FLuaLocalValue LuaLocalValue
        );
    
    // Lua로부터 Template 함수를 가져온다.
    // 해당 함수가 없으면 Throw한다.
    FLuaTemplateFunctions GetTemplateFunctionFromScript(
        sol::environment& InEnv
    );
    
    void SetLuaScriptField(
        fs::path Path,
        sol::environment& InEnv,
        sol::table& InScriptTable,
        FLuaTemplateFunctions& InLuaTemplateFunction
    );
    
    FScript* GetOrCreate(FString InScriptName);
private:
    const static inline FString SCRIPT_FILE_PATH{"Scripts/"};
    const static inline FString DEFAULT_FILE_PATH{"Scripts/template.lua"};
private:
    sol::state Lua;
    
    // 소유자 기반 접근
    TMap<AActor*, TArray<FScript*>> ScriptsByOwner;
};