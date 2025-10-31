#pragma once

struct FLuaTemplateFunctions
{
    sol::function BeginPlay;
    sol::function EndPlay;
    sol::function OnOverlap;
    sol::function Tick;
};

struct FScript
{
    FString ScriptName;

    sol::environment Env;
    sol::table Table;
    FLuaTemplateFunctions LuaTemplateFunctions;
};

class UScriptManager : public UObject
{
    DECLARE_CLASS(UScriptManager, UObject);
    /*
     pseudo code

     void AttachScriptTo(AActor* Target, FString ScriptName)
     {
        auto Script = Scripts.find()
        if ()
        {
            Scripts[Target] = CreateDefaultScript(ScriptName);
            UE_LOG("ScriptManager : Created %s script and attached it to %s.",\
            ScriptName, Target.GetName());
        }
        else
        {
            
        }
     }

     TSet<Script> Scripts;
     TMap<AActor*, LuaData> Scripts;
     */

public:
    /*
     Single Ton Pattern
     이라 원래 Private에 있어야 하지만 IMPLEMENT_CLASS 이슈로
     일단 public에 넣습니다.
     TODO: 추후 수정해야 함.
    */
    UScriptManager();
    ~UScriptManager() override;

    static UScriptManager& GetInstance();

    void Initialize();
    void AttachScriptTo(AActor* Target, FString ScriptName)
    {
        TMap<std::string, FScript*>::iterator ScriptFound =
            ScriptsByName.find(ScriptName);
        
        if (ScriptFound == ScriptsByName.end())
        {
            // template.lua를 복사하여 Script를 새로 생성한 후 할당
        }
        else
        {
            ScriptsByOwner[Target] = ScriptFound->second;
        }
    }

private:
    // Lua로부터 Template 함수를 가져온다.
    // 해당 함수가 없으면 Throw한다.
    FLuaTemplateFunctions GetTemplateFunctionFromScript(
        sol::environment& InEnv
    );
private:
    const static inline FString ScriptsFilePath{"Scripts"};
private:
    sol::state Lua;
    TMap<FString, FScript*> ScriptsByName; 
    TMap<AActor*, FScript*> ScriptsByOwner;
};